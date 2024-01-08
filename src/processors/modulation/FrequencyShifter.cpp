#include "FrequencyShifter.h"
#include "gui/utils/ModulatableSlider.h"
#include "processors/BufferHelpers.h"
#include "processors/ParameterHelpers.h"

namespace freq_shifter_tags
{
const juce::String shiftTag = "shift";
const juce::String fbTag = "feedback";
const juce::String spreadTag = "spread";
const juce::String mixTag = "mix";
const juce::String modeTag = "mode";
} // namespace freq_shifter_tags

FrequencyShifter::FrequencyShifter (UndoManager* um)
    : BaseProcessor ("Laser Trem", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    using namespace freq_shifter_tags;
    loadParameterPointer (shiftParam, vts, shiftTag);
    loadParameterPointer (spreadParam, vts, spreadTag);
    loadParameterPointer (modeParam, vts, modeTag);

    feedback.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, fbTag));
    feedback.setRampLength (0.05);
    feedback.mappingFunction = [] (float fb01)
    {
        return 0.95f * std::sqrt (fb01);
    };

    dryGain.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, mixTag));
    dryGain.setRampLength (0.05);
    dryGain.mappingFunction = [] (float mix)
    {
        return std::sin (0.5f * MathConstants<float>::pi * (1.0f - mix));
    };

    wetGain.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, mixTag));
    wetGain.setRampLength (0.05);
    wetGain.mappingFunction = [] (float mix)
    {
        return std::sin (0.5f * MathConstants<float>::pi * (mix));
    };

    addPopupMenuParameter (modeTag);

    uiOptions.backgroundColour = Colours::palevioletred.brighter (0.1f);
    uiOptions.powerColour = Colours::yellow;
    uiOptions.info.description = "A tremolo-style effect using frequency shifting.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout FrequencyShifter::createParameterLayout()
{
    using namespace ParameterHelpers;
    using namespace freq_shifter_tags;

    auto params = createBaseParams();

    emplace_param<chowdsp::FloatParameter> (params,
                                            shiftTag,
                                            "Shift",
                                            juce::NormalisableRange { -20.0f, 20.0f },
                                            0.0f,
                                            &floatValToString,
                                            &stringToFloatVal);
    createPercentParameter (params, fbTag, "Feedback", 0.0f);
    createBipolarPercentParameter (params, spreadTag, "Spread", 0.5f);
    createPercentParameter (params, mixTag, "Mix", 1.0f);

    emplace_param<chowdsp::EnumChoiceParameter<Mode>> (params, modeTag, "Mode", Mode::Plain);

    return { params.begin(), params.end() };
}

void FrequencyShifter::prepare (double sampleRate, int samplesPerBlock)
{
    const auto monoSpec = dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 1 };
    const auto stereoSpec = dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 2 };

    outputBuffer.setSize (2, samplesPerBlock);

    feedback.prepare (sampleRate, samplesPerBlock);
    dryGain.prepare (sampleRate, samplesPerBlock);
    wetGain.prepare (sampleRate, samplesPerBlock);
    fbData = {};

    lfoData.resize (2 * static_cast<size_t> (samplesPerBlock), {});

    for (auto& filter : hilbertFilter)
        filter.reset();
    for (auto& modulator : modulators)
        modulator.prepare (monoSpec);

    dcBlocker.prepare (stereoSpec);
    dcBlocker.calcCoefs (20.0f, static_cast<float> (sampleRate));

    prevMode = modeParam->get();
}

void FrequencyShifter::processAudio (AudioBuffer<float>& buffer)
{
    const auto mode = modeParam->get();
    if (mode != prevMode)
    {
        prevMode = mode;
        fbData = {};
        for (auto& modulator : modulators)
            modulator.reset();
        for (auto& filter : hilbertFilter)
            filter.reset();
    }

    const auto numInChannels = buffer.getNumChannels();
    const auto numOutChannels = mode == Mode::Plain ? numInChannels : 2;
    const auto numSamples = buffer.getNumSamples();

    outputBuffer.setSize (numOutChannels, numSamples, false, false, true);
    dryBuffer.setSize (numOutChannels, numSamples, false, false, true);

    for (auto [ch, dryData] : chowdsp::buffer_iters::channels (dryBuffer))
    {
        juce::FloatVectorOperations::copy (dryData.data(), buffer.getReadPointer (ch % numInChannels), numSamples);
    }

    feedback.process (numSamples);
    if (mode == Mode::Plain || mode == Mode::Stereo)
    {
        processPlainOrStereo (buffer, outputBuffer, mode == Mode::Stereo);
    }
    else
    {
        BufferHelpers::collapseToMonoBuffer (buffer, buffer);
        processSpread (buffer, outputBuffer);
    }

    dcBlocker.processBlock (outputBuffer);

    dryGain.process (numSamples);
    wetGain.process (numSamples);
    for (auto [ch, dryData, wetData] : chowdsp::buffer_iters::zip_channels (std::as_const (dryBuffer), outputBuffer))
    {
        juce::FloatVectorOperations::multiply (wetData.data(), wetGain.getSmoothedBuffer(), numSamples);
        juce::FloatVectorOperations::addWithMultiply (wetData.data(), dryData.data(), dryGain.getSmoothedBuffer(), numSamples);
    }

    outputBuffers.getReference (0) = &outputBuffer;
}

void FrequencyShifter::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto mode = modeParam->get();
    if (mode != prevMode)
    {
        prevMode = mode;
        fbData = {};
        for (auto& modulator : modulators)
            modulator.reset();
        for (auto& filter : hilbertFilter)
            filter.reset();
    }

    const auto numInChannels = buffer.getNumChannels();
    const auto numOutChannels = numInChannels;
    const auto numSamples = buffer.getNumSamples();

    outputBuffer.setSize (numOutChannels, numSamples, false, false, true);

    feedback.process (numSamples);
    dryGain.process (numSamples);
    wetGain.process (numSamples);
    chowdsp::BufferMath::copyBufferData (buffer, outputBuffer);

    outputBuffers.getReference (0) = &outputBuffer;
}

static auto processFreqShifterModulator (chowdsp::SineWave<float>& modulator,
                                         float modFrequencyHz,
                                         std::span<float> lfoData,
                                         int numSamples) noexcept
{
    modulator.setFrequency (modFrequencyHz);

    auto* lfoSinData = lfoData.data();
    auto* lfoCosData = lfoData.data() + numSamples;
    for (int n = 0; n < numSamples; ++n)
    {
        std::tie (lfoSinData[n], lfoCosData[n]) = modulator.processSampleQuadrature();
    }

    return std::make_pair (lfoSinData, lfoCosData);
}

static auto processFreqShifterSample (float x,
                                      float& z,
                                      float fbMult,
                                      chowdsp::HilbertFilter<float>& hilbert,
                                      std::pair<float, float> lfoData) noexcept
{
    x = x + z * fbMult;
    const auto [x_re, x_im] = hilbert.process (x);

    z = x_re * lfoData.first + x_im * lfoData.second;

    return z;
}

void FrequencyShifter::processPlainOrStereo (chowdsp::BufferView<const float> bufferIn,
                                             chowdsp::BufferView<float> bufferOut,
                                             bool isStereo) noexcept
{
    const auto numInChannels = bufferIn.getNumChannels();
    const auto numOutChannels = bufferOut.getNumChannels();
    const auto numSamples = bufferOut.getNumSamples();

    auto [lfoSinData, lfoCosData] = processFreqShifterModulator (modulators[0],
                                                                 shiftParam->getCurrentValue(),
                                                                 lfoData,
                                                                 numSamples);

    for (auto ch = 0; ch < numOutChannels; ++ch)
    {
        const auto* dataIn = bufferIn.getReadPointer (ch % numInChannels);
        auto* dataOut = bufferOut.getWritePointer (ch);

        if (ch == 1 && isStereo)
        {
            juce::FloatVectorOperations::negate (lfoSinData, lfoSinData, numSamples);
            juce::FloatVectorOperations::negate (lfoCosData, lfoCosData, numSamples);
        }

        const auto* feedbackSmooth = feedback.getSmoothedBuffer();
        chowdsp::ScopedValue z { fbData[(size_t) ch] };
        for (int n = 0; n < numSamples; ++n)
        {
            dataOut[n] = processFreqShifterSample (dataIn[n],
                                                   z.get(),
                                                   feedbackSmooth[n],
                                                   hilbertFilter[(size_t) ch],
                                                   { lfoSinData[n], lfoCosData[n] });
        }
    }
}

void FrequencyShifter::processSpread (chowdsp::BufferView<const float> bufferIn,
                                      chowdsp::BufferView<float> bufferOut) noexcept
{
    const auto numSamples = bufferOut.getNumSamples();
    const auto* dataIn = bufferIn.getReadPointer (0);
    auto* dataOutLeft = bufferOut.getWritePointer (0);
    auto* dataOutRight = bufferOut.getWritePointer (1);
    bufferOut.clear();

    const auto processVoice = [&, spreadFactor = std::exp2 (spreadParam->getCurrentValue())] (size_t voiceIndex, std::initializer_list<float*> outPtrs)
    {
        const auto voiceModFrequency = shiftParam->getCurrentValue() * std::pow (spreadFactor, (float) voiceIndex - 1.0f);
        auto [lfoSinData, lfoCosData] = processFreqShifterModulator (modulators[voiceIndex],
                                                                     voiceModFrequency,
                                                                     lfoData,
                                                                     numSamples);

        const auto* feedbackSmooth = feedback.getSmoothedBuffer();
        chowdsp::ScopedValue z { fbData[voiceIndex] };
        for (int n = 0; n < numSamples; ++n)
        {
            const auto y = processFreqShifterSample (dataIn[n],
                                                     z.get(),
                                                     feedbackSmooth[n],
                                                     hilbertFilter[voiceIndex],
                                                     { lfoSinData[n], lfoCosData[n] });
            for (auto ptr : outPtrs)
                ptr[n] += y;
        }
    };

    processVoice (1, { dataOutLeft, dataOutRight });
    chowdsp::BufferMath::applyGain (bufferOut, 0.5f);
    processVoice (0, { dataOutLeft });
    processVoice (2, { dataOutRight });
}

bool FrequencyShifter::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;
    using namespace freq_shifter_tags;
    class SpreadControl : public ModulatableSlider
    {
    public:
        SpreadControl (AudioProcessorValueTreeState& vts, chowdsp::HostContextProvider& hcp)
            : ModulatableSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, spreadTag), hcp),
              spreadAttach (vts, spreadTag, *this),
              modeParam (getParameterPointer<chowdsp::EnumChoiceParameter<Mode>*> (vts, modeTag)),
              modeAttach (
                  *modeParam,
                  [this] (float newValue)
                  { updateControlVisibility (magic_enum::enum_value<Mode> ((size_t) newValue)); },
                  vts.undoManager)
        {
            hcp.registerParameterComponent (*this, getParameter());
            Component::setName (spreadTag + "__");
        }

        void updateControlVisibility (Mode mode)
        {
            setEnabled (mode == Mode::Spread);
        }

        void visibilityChanged() override
        {
            Component::setName ("Spread");
            updateControlVisibility (modeParam->get());
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
        SliderAttachment spreadAttach;

        chowdsp::EnumChoiceParameter<Mode>* modeParam = nullptr;
        ParameterAttachment modeAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpreadControl)
    };

    customComps.add (std::make_unique<SpreadControl> (vts, hcp));

    return false;
}
