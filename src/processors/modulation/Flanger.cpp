#include "Flanger.h"
#include "../BufferHelpers.h"
#include "../ParameterHelpers.h"

namespace
{
constexpr float rateLow = 0.5f;
constexpr float rateHigh = 40.0f;
constexpr float delayMs = 0.001f;

const String delayTypeTag = "delay_type";
} // namespace

Flanger::Flanger (UndoManager* um) : BaseProcessor ("Flanger",
                                                    createParameterLayout(),
                                                    um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (rateParam, vts, "rate");
    loadParameterPointer (delayAmountParam, vts, "delayAmount");
    loadParameterPointer (delayOffsetParam, vts, "delayOffset");
    loadParameterPointer (fbParam, vts, "feedback");
    loadParameterPointer (mixParam, vts, "mix");
    delayTypeParam = vts.getRawParameterValue (delayTypeTag);

    addPopupMenuParameter (delayTypeTag);

    uiOptions.backgroundColour = Colour (106, 102, 190);
    uiOptions.powerColour = Colours::yellow.brighter (0.1f);
    uiOptions.info.description = "A flanger effect. Use the right-click menu to enable lo-fi mode.";
    uiOptions.info.authors = StringArray { "Kai Mikkelsen", "Jatin Chowdhury" };

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ "rate" }, ModulationInput);
}

ParamLayout Flanger::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    auto delayAmountRange = juce::NormalisableRange { 0.0f, 20.0f };
    delayAmountRange.setSkewForCentre (4.5f);
    auto delayOffsetRange = juce::NormalisableRange { 0.0f, 20.0f };
    delayOffsetRange.setSkewForCentre (4.5f);

    createPercentParameter (params, "rate", "Rate", 0.5f);
    createTimeMsParameter (params, "delayAmount", "Amount", delayAmountRange, 2.0f);
    createTimeMsParameter (params, "delayOffset", "Offset", delayOffsetRange, 1.0f);
    createPercentParameter (params, "feedback", "Feedback", 0.0f);
    createPercentParameter (params, "mix", "Mix", 0.5f);

    emplace_param<AudioParameterChoice> (params, delayTypeTag, "Delay Type", StringArray { "Clean", "Lo-Fi" }, 0);

    return { params.begin(), params.end() };
}

void Flanger::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    dsp::ProcessSpec monoSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    fs = (float) sampleRate;

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < delaysPerChannel; ++i)
        {
            cleanDelay[ch][i].prepare (monoSpec);
            lofiDelay[ch][i].prepare (monoSpec);

            LFOs[ch][i].prepare (monoSpec);
            LFOData[ch][i].resize ((size_t) samplesPerBlock, 0.0f);
        }

        smooth[ch].reset (sampleRate, 0.05);

        feedbackState[ch] = 0.0f;
        fbSmooth[ch].reset (sampleRate, 0.01);

        delaySmoothSamples[ch].reset (sampleRate, 0.01);
        delayOffsetSmoothSamples[ch].reset (sampleRate, 0.01);
    }

    // set phase offset
    constexpr float piOver2 = MathConstants<float>::pi / 2.0f;
    LFOs[1][0].reset (piOver2);

    aaFilter.prepare (spec);
    aaFilter.setCutoffFrequency (12000.0f);

    dryWetMixer.prepare (spec);
    dryWetMixer.setMixingRule (dsp::DryWetMixingRule::sin3dB);

    dcBlocker.prepare (spec);
    dcBlocker.setCutoffFrequency (60.0f);

    audioOutBuffer.setSize (2, samplesPerBlock);
    modOutBuffer.setSize (1, samplesPerBlock);

    for (auto& filt : hilbertFilter)
        filt.reset();

    bypassNeedsReset = false;
}

void Flanger::processModulation (int numSamples)
{
    modOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (ModulationInput))
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);

        auto phaseShiftLFO = [this, numSamples, filterIndex = 0] (float* lfoIn, float* lfoOut) mutable
        {
            // trying to use more of the Hilbert filters than we have available!
            jassert (filterIndex < (int) std::size (hilbertFilter));

            auto& filter = hilbertFilter[filterIndex++];
            for (int n = 0; n < numSamples; ++n)
                std::tie (lfoIn[n], lfoOut[n]) = filter.process (lfoIn[n]);
        };

        auto* modOutData = modOutBuffer.getReadPointer (0);

        FloatVectorOperations::copy (LFOData[0][0].data(), modOutData, numSamples);
        phaseShiftLFO (LFOData[0][0].data(), LFOData[1][0].data());
    }
    else
    {
        auto rate = rateLow * std::pow (rateHigh / rateLow, *rateParam);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < delaysPerChannel; ++i)
            {
                LFOs[ch][i].setFrequency (rate);
            }

            for (int i = 0; i < delaysPerChannel; ++i)
            {
                auto* data = LFOData[ch][i].data();
                for (int n = 0; n < numSamples; ++n)
                    data[n] = LFOs[ch][i].processSample();
            }
        }

        auto* modOutData = modOutBuffer.getWritePointer (0);
        FloatVectorOperations::copy (modOutData, LFOData[0][0].data(), numSamples);
    }
}

template <typename DelayArrType>
void Flanger::processFlanger (AudioBuffer<float>& buffer, DelayArrType& delay)
{
    jassert (buffer.getNumChannels() == 2); // always processing in stereo
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < delaysPerChannel; ++i)
            delay[ch][i].setFilterFreq (10000.0f);

        auto fbAmount = std::sqrt (fbParam->getCurrentValue());
        if constexpr (std::is_same_v<DelayArrType, decltype (lofiDelay)>)
            fbAmount *= 0.4f;
        else
            fbAmount *= 0.5f;

        fbSmooth[ch].setTargetValue (fbAmount);

        delaySmoothSamples[ch].setTargetValue (delayMs * fs * delayAmountParam->getCurrentValue());
        delayOffsetSmoothSamples[ch].setTargetValue (delayMs * fs * delayOffsetParam->getCurrentValue());

        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < numSamples; ++n)
        {
            auto xIn = std::tanh (x[n] * 0.75f - feedbackState[ch]);

            auto delayValue = delaySmoothSamples[ch].getNextValue();
            auto delayOffsetValue = delayOffsetSmoothSamples[ch].getNextValue();

            x[n] = 0.0f;
            for (int i = 0; i < delaysPerChannel; ++i)
            {
                float delayAmt = delayValue + delayOffsetValue * 0.5f * (1.0f + 0.95f * LFOData[ch][i][(size_t) n]);

                delay[ch][i].setDelay (delayAmt);
                delay[ch][i].pushSample (0, xIn);
                x[n] += delay[ch][i].popSample (0);
            }

            x[n] = aaFilter.processSample (ch, x[n]);

            feedbackState[ch] = fbSmooth[ch].getNextValue() * x[n];
            feedbackState[ch] = dcBlocker.processSample (ch, feedbackState[ch]);
        }
    }
}

void Flanger::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    processModulation (numSamples);

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numInChannels = audioInBuffer.getNumChannels();

        // always have a stereo output!
        audioOutBuffer.setSize (2, numSamples, false, false, true);
        audioOutBuffer.copyFrom (0, 0, audioInBuffer, 0, 0, numSamples);
        audioOutBuffer.copyFrom (1, 0, audioInBuffer, 1 % numInChannels, 0, numSamples);

        dsp::AudioBlock<float> block { audioOutBuffer };

        dryWetMixer.setWetMixProportion (*mixParam);
        dryWetMixer.pushDrySamples (block);

        const auto delayTypeIndex = (int) *delayTypeParam;
        if (delayTypeIndex != prevDelayTypeIndex)
        {
            for (auto& delaySet : cleanDelay)
                for (auto& delay : delaySet)
                    delay.reset();

            for (auto& delaySet : lofiDelay)
                for (auto& delay : delaySet)
                    delay.reset();

            prevDelayTypeIndex = delayTypeIndex;
        }

        if (delayTypeIndex == 0)
            processFlanger (audioOutBuffer, cleanDelay);
        else if (delayTypeIndex == 1)
            processFlanger (audioOutBuffer, lofiDelay);

        dryWetMixer.mixWetSamples (block);
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;

    bypassNeedsReset = true;
}

void Flanger::processAudioBypassed (AudioBuffer<float>& buffer)
{
    if (bypassNeedsReset)
    {
        for (auto& delaySet : cleanDelay)
            for (auto& delay : delaySet)
                delay.reset();

        for (auto& delaySet : lofiDelay)
            for (auto& delay : delaySet)
                delay.reset();

        bypassNeedsReset = false;
    }

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
