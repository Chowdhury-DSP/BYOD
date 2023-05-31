#include "Tremolo.h"
#include "../BufferHelpers.h"
#include "../ParameterHelpers.h"

namespace
{
template <typename SampleType, typename SmoothingType>
static dsp::AudioBlock<SampleType>& addSmoothed (dsp::AudioBlock<SampleType>& block, SmoothedValue<SampleType, SmoothingType>& value) noexcept
{
    if (! value.isSmoothing())
        return block.add (value.getTargetValue());

    for (size_t i = 0; i < block.getNumSamples(); ++i)
    {
        const auto scaler = value.getNextValue();
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            block.getChannelPointer (ch)[i] += scaler;
    }

    return block;
}

const String stereoTag = "stereo";
const String v1WaveTag = "v1_wave";

} // namespace

Tremolo::Tremolo (UndoManager* um) : BaseProcessor ("Tremolo",
                                                    createParameterLayout(),
                                                    um,
                                                    magic_enum::enum_count<InputPort>(),
                                                    magic_enum::enum_count<OutputPort>())
{
    using namespace ParameterHelpers;
    loadParameterPointer (rateParam, vts, "rate");
    loadParameterPointer (waveParam, vts, "wave");
    loadParameterPointer (depthParam, vts, "depth");
    loadParameterPointer (stereoParam, vts, stereoTag);
    loadParameterPointer (v1WaveParam, vts, v1WaveTag);

    addPopupMenuParameter (stereoTag);
    addPopupMenuParameter (v1WaveTag);

    uiOptions.backgroundColour = Colours::orange.darker (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter();
    uiOptions.info.description = "A simple tremolo effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ "rate", "wave" }, ModulationInput);
}

ParamLayout Tremolo::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createFreqParameter (params, "rate", "Rate", 2.0f, 20.0f, 10.0f, 10.0f);
    createPercentParameter (params, "wave", "Wave", 0.5f);
    createPercentParameter (params, "depth", "Depth", 0.5f);

    emplace_param<chowdsp::BoolParameter> (params, stereoTag, "Stereo", false);
    emplace_param<chowdsp::BoolParameter> (params, v1WaveTag, "V1 Wave", false);

    return { params.begin(), params.end() };
}

void Tremolo::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec monoSpec { sampleRate, (uint32) samplesPerBlock, 1 };

    filter.prepare (monoSpec);
    filter.setCutoffFrequency (250.0f);

    modOutBuffer.setSize (1, samplesPerBlock);
    audioOutBuffer.setSize (2, samplesPerBlock);

    phaseSmooth.setRampLength (0.01);
    waveSmooth.setRampLength (0.01);
    depthGainSmooth.setRampLength (0.01);
    depthAddSmooth.setRampLength (0.01);

    phaseSmooth.prepare (sampleRate, samplesPerBlock);
    waveSmooth.prepare (sampleRate, samplesPerBlock);
    depthGainSmooth.prepare (sampleRate, samplesPerBlock);
    depthAddSmooth.prepare (sampleRate, samplesPerBlock);

    fs = (float) sampleRate;
    phase = 0.0f;
}

void Tremolo::fillWaveBuffer_old (float* waveBuff, const int numSamples, float& p)
{
    bool isSmoothing = phaseSmooth.isSmoothing() || waveSmooth.isSmoothing();

    if (isSmoothing)
    {
        const auto* phaseSmoothData = phaseSmooth.getSmoothedBuffer();
        const auto* waveSmoothData = waveSmooth.getSmoothedBuffer();
        for (int n = 0; n < numSamples; ++n)
        {
            auto curWave = waveSmoothData[n];
            auto sineGain = 1.0f - jmin (2.0f * curWave, 1.0f);
            auto squareGain = jmax (2.0f * (curWave - 0.5f), 0.0f);
            auto triGain = 1.0f - 2.0f * std::abs (0.5f - curWave);

            waveBuff[n] = sineGain * dsp::FastMathApproximations::sin (p); // sine
            waveBuff[n] += triGain * p / MathConstants<float>::pi; // triangle
            waveBuff[n] += squareGain * (p > 0.0f ? 1.0f : -1.0f); // square

            p += phaseSmoothData[n];
            p = p > MathConstants<float>::pi ? p - MathConstants<float>::twoPi : p;
        }
    }
    else
    {
        auto phaseInc = phaseSmooth.getCurrentValue();
        auto curWave = waveSmooth.getCurrentValue();
        if (curWave <= 0.5f)
        {
            auto sineGain = 1.0f - jmin (2.0f * curWave, 1.0f);
            auto triGain = 1.0f - 2.0f * std::abs (0.5f - curWave);
            for (int n = 0; n < numSamples; ++n)
            {
                waveBuff[n] = sineGain * dsp::FastMathApproximations::sin (p); // sine
                waveBuff[n] += triGain * p / MathConstants<float>::pi; // triangle

                p += phaseInc;
                p = p > MathConstants<float>::pi ? p - MathConstants<float>::twoPi : p;
            }
        }
        else
        {
            auto squareGain = jmax (2.0f * (curWave - 0.5f), 0.0f);
            auto triGain = 1.0f - 2.0f * std::abs (0.5f - curWave);
            for (int n = 0; n < numSamples; ++n)
            {
                waveBuff[n] = triGain * p / MathConstants<float>::pi; // triangle
                waveBuff[n] += squareGain * (p > 0.0f ? 1.0f : -1.0f); // square

                p += phaseInc;
                p = p > MathConstants<float>::pi ? p - MathConstants<float>::twoPi : p;
            }
        }
    }
}

void Tremolo::fillWaveBuffer (float* waveBuff, const int numSamples, float& p)
{
    const auto triangle_saw = [] (float p_o_pi, float s)
    {
        const auto b = p_o_pi + 1.0f - s;
        return (2.0f * std::abs (b)) / (1.0f - chowdsp::Math::sign (b) * (s - 1.0f)) - 1.0f;
    };

    const auto saw_square = [] (float p_o_pi, float s)
    {
        return std::clamp (p_o_pi / (1.0f - s), -1.0f, 1.0f);
    };

    const auto* phaseSmoothData = phaseSmooth.getSmoothedBuffer();
    const auto* waveSmoothData = waveSmooth.getSmoothedBuffer();
    for (int n = 0; n < numSamples; ++n)
    {
        const auto curWave = waveSmoothData[n];
        const auto p_over_pi = p / juce::MathConstants<float>::pi;

        if (curWave < 1.0f / 3.0f)
        {
            const auto mixTriangle = std::min (3.0f * curWave, 1.0f);
            waveBuff[n] = mixTriangle * triangle_saw (p_over_pi, 1.0f) + (1.0f - mixTriangle) * std::sin (p - juce::MathConstants<float>::halfPi);
        }
        else if (curWave < 2.0f / 3.0f)
        {
            waveBuff[n] = triangle_saw (p_over_pi, 1.0f - std::min (3.0f * curWave - 1.0f, 1.0f));
        }
        else
        {
            waveBuff[n] = saw_square (p_over_pi, std::min (3.0f * curWave - 2.0f, 1.0f));
        }

        p += phaseSmoothData[n];
        p = p > MathConstants<float>::pi ? p - MathConstants<float>::twoPi : p;
    }
}

void Tremolo::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    modOutBuffer.setSize (1, numSamples, false, false, true);

    phaseSmooth.process (*rateParam * MathConstants<float>::pi / fs, numSamples);
    waveSmooth.process (*waveParam, numSamples);

    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
    }
    else // create our own modulation signal
    {
        // fill modulation buffer (-1, 1)
        if (! v1WaveParam->get())
            fillWaveBuffer (modOutBuffer.getWritePointer (0), numSamples, phase);
        else
            fillWaveBuffer_old (modOutBuffer.getWritePointer (0), numSamples, phase);

        // smooth out modulation signal
        auto&& modBlock = dsp::AudioBlock<float> { modOutBuffer };
        filter.process (dsp::ProcessContextReplacing<float> { modBlock });
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto stereoMode = stereoParam->get();
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numInChannels = audioInBuffer.getNumChannels();
        const auto numOutChannels = stereoMode ? 2 : numInChannels;

        audioOutBuffer.setSize (numOutChannels, numSamples, false, false, true);

        // copy modulation data into channel 0 of audio output buffer, and shrink range to (0, 1)
        audioOutBuffer.copyFrom (0, 0, modOutBuffer.getReadPointer (0), numSamples, 0.5f);
        FloatVectorOperations::add (audioOutBuffer.getWritePointer (0), 0.5f, numSamples);

        // apply depth parameter
        {
            auto&& waveBlock = dsp::AudioBlock<float> { audioOutBuffer }.getSingleChannelBlock (0);
            auto depthVal = std::pow (depthParam->getCurrentValue(), 0.33f);
            depthGainSmooth.process (depthVal, numSamples);
            juce::FloatVectorOperations::multiply (waveBlock.getChannelPointer (0), depthGainSmooth.getSmoothedBuffer(), numSamples);
            depthAddSmooth.process (1.0f - depthVal, numSamples);
            juce::FloatVectorOperations::add (waveBlock.getChannelPointer (0), depthAddSmooth.getSmoothedBuffer(), numSamples);
        }

        // copy modulation data into all the channels
        for (int ch = 1; ch < numOutChannels; ++ch)
        {
            if (stereoMode)
            {
                FloatVectorOperations::negate (audioOutBuffer.getWritePointer (ch), audioOutBuffer.getReadPointer (0), numSamples);
                FloatVectorOperations::add (audioOutBuffer.getWritePointer (ch), 1.0f, numSamples);
            }
            else
            {
                audioOutBuffer.copyFrom (ch, 0, audioOutBuffer, 0, 0, numSamples);
            }
        }

        // multiply with incoming audio data
        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            const auto* x = audioInBuffer.getReadPointer (ch % numInChannels);
            auto* y = audioOutBuffer.getWritePointer (ch);
            FloatVectorOperations::multiply (y, x, numSamples);
        }
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}

void Tremolo::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    modOutBuffer.setSize (1, numSamples, false, false, true);

    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
    }
    else
    {
        modOutBuffer.clear();
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);
        audioOutBuffer.makeCopyOf (audioInBuffer, true);
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}

void Tremolo::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);

    using namespace std::string_view_literals;
    if (version <= chowdsp::Version { "1.1.7"sv })
    {
        // In Version 1.1.8 we made some changes to how the LFO shape interacts with the
        // "Wave" parameter. So for versions 1.1.7 and earlier, we need to revert to the
        // v1 behaviour.
        v1WaveParam->setValueNotifyingHost (1.0f);
    }
}
