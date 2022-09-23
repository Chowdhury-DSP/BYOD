#include "Delay.h"
#include "../ParameterHelpers.h"

namespace
{
const String delayTypeTag = "delay_type";
const String pingPongTag = "ping_pong";
} // namespace

DelayModule::DelayModule (UndoManager* um) : BaseProcessor ("Delay", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (delayTimeMsParam, vts, "time_ms");
    loadParameterPointer (freqParam, vts, "freq");
    loadParameterPointer (feedbackParam, vts, "feedback");
    loadParameterPointer (mixParam, vts, "mix");
    delayTypeParam = vts.getRawParameterValue (delayTypeTag);
    pingPongParam = vts.getRawParameterValue (pingPongTag);

    addPopupMenuParameter (delayTypeTag);
    addPopupMenuParameter (pingPongTag);

    uiOptions.backgroundColour = Colours::cyan.darker (0.1f);
    uiOptions.powerColour = Colours::gold;
    uiOptions.info.description = "A simple delay effect with feedback. Use the right-click menu to enable lo-fi mode or a ping-pong effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout DelayModule::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createTimeMsParameter (params, "time_ms", "Delay Time", createNormalisableRange (20.0f, 2000.0f, 200.0f), 100.0f);

    createFreqParameter (params, "freq", "Cutoff", 500.0f, 10000.0f, 4000.0f, 10000.0f);
    createPercentParameter (params, "feedback", "Feedback", 0.0f);
    createPercentParameter (params, "mix", "Mix", 0.5f);

    emplace_param<AudioParameterChoice> (params, delayTypeTag, "Delay Type", StringArray { "Clean", "Lo-Fi" }, 0);
    emplace_param<AudioParameterBool> (params, pingPongTag, "Ping-Pong", false);

    return { params.begin(), params.end() };
}

void DelayModule::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;
    stereoBuffer.setSize (2, samplesPerBlock);

    dsp::ProcessSpec stereoSpec { sampleRate, (uint32) samplesPerBlock, 2 };
    dsp::ProcessSpec monoSpec = stereoSpec;
    monoSpec.numChannels = 1;

    cleanDelayLine.prepare (stereoSpec);
    lofiDelayLine.prepare (stereoSpec);

    dryWetMixer.prepare (stereoSpec);
    dryWetMixer.setMixingRule (dsp::DryWetMixingRule::balanced);
    dryWetMixerMono.prepare (monoSpec);
    dryWetMixerMono.setMixingRule (dsp::DryWetMixingRule::balanced);

    delaySmooth.reset (sampleRate, 0.1);
    freqSmooth.reset (sampleRate, 0.1);

    feedbackSmoothBuffer.prepare (sampleRate, samplesPerBlock);
    feedbackSmoothBuffer.setRampLength (0.01);

    bypassNeedsReset = false;
}

template <typename DelayType>
void DelayModule::processMonoStereoDelay (AudioBuffer<float>& buffer, DelayType& delayLine)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    dsp::AudioBlock<float> block { buffer };

    auto& dryWet = numChannels == 1 ? dryWetMixerMono : dryWetMixer;
    dryWet.setWetMixProportion (*mixParam * 0.5f);
    dryWet.pushDrySamples (block);

    const auto* fbData = feedbackSmoothBuffer.getSmoothedBuffer();
    if (delaySmooth.isSmoothing() || freqSmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            auto* x = buffer.getWritePointer (0);
            for (int n = 0; n < numSamples; ++n)
            {
                delayLine.setDelay (delaySmooth.getNextValue());
                delayLine.setFilterFreq (freqSmooth.getNextValue());

                auto y = delayLine.popSample (0);
                delayLine.pushSample (0, x[n] + y * fbData[n]);
                x[n] = y;
            }
        }
        else
        {
            auto* xL = buffer.getWritePointer (0);
            auto* xR = buffer.getWritePointer (1);
            for (int n = 0; n < numSamples; ++n)
            {
                delayLine.setDelay (delaySmooth.getNextValue());
                delayLine.setFilterFreq (freqSmooth.getNextValue());

                auto y = delayLine.popSample (0);
                delayLine.pushSample (0, xL[n] + y * fbData[n]);
                xL[n] = y;

                y = delayLine.popSample (1);
                delayLine.pushSample (1, xR[n] + y * fbData[n]);
                xR[n] = y;
            }
        }
    }
    else
    {
        delayLine.setDelay (delaySmooth.getTargetValue());
        delayLine.setFilterFreq (freqSmooth.getTargetValue());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < numSamples; ++n)
            {
                auto y = delayLine.popSample (ch);
                delayLine.pushSample (ch, x[n] + y * fbData[n]);
                x[n] = y;
            }
        }
    }

    dryWet.mixWetSamples (block);
    outputBuffers.getReference (0) = &buffer;
}

template <typename DelayType>
void DelayModule::processPingPongDelay (AudioBuffer<float>& buffer, DelayType& delayLine)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    // if we have a mono input, copy it into a stereo buffer first
    if (numChannels == 1)
    {
        stereoBuffer.setSize (2, numSamples, false, false, true);
        stereoBuffer.clear();

        stereoBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
        stereoBuffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    auto& bufferToProcess = numChannels == 2 ? buffer : stereoBuffer;

    dsp::AudioBlock<float> block { bufferToProcess };
    dryWetMixer.setWetMixProportion (*mixParam * 0.5f);
    dryWetMixer.pushDrySamples (block);

    // if we have a stereo input collapse to mono
    if (numChannels == 2)
    {
        bufferToProcess.addFrom (0, 0, bufferToProcess, 1, 0, numSamples);
        bufferToProcess.applyGain (0, 0, numSamples, 0.5f);
    }

    auto* dataL = bufferToProcess.getWritePointer (0);
    auto* dataR = bufferToProcess.getWritePointer (1);

    const auto* fbData = feedbackSmoothBuffer.getSmoothedBuffer();
    auto processSample = [&] (int n)
    {
        auto yL = delayLine.popSample (0);
        auto yR = delayLine.popSample (1);

        auto xL = dataL[n] + dataR[n] + fbData[n] * yR;
        auto xR = fbData[n] * yL;

        delayLine.pushSample (0, xL);
        delayLine.pushSample (1, xR);

        dataL[n] = yL;
        dataR[n] = yR;
    };

    if (delaySmooth.isSmoothing() || freqSmooth.isSmoothing())
    {
        for (int n = 0; n < numSamples; ++n)
        {
            delayLine.setDelay (delaySmooth.getNextValue());
            delayLine.setFilterFreq (freqSmooth.getNextValue());
            processSample (n);
        }
    }
    else
    {
        delayLine.setDelay (delaySmooth.getTargetValue());
        delayLine.setFilterFreq (freqSmooth.getTargetValue());

        for (int n = 0; n < numSamples; ++n)
            processSample (n);
    }

    dryWetMixer.mixWetSamples (block);

    // tell future processors to use stereo buffer
    outputBuffers.getReference (0) = &bufferToProcess;
}

void DelayModule::processAudio (AudioBuffer<float>& buffer)
{
    feedbackSmoothBuffer.process (std::pow (feedbackParam->getCurrentValue() * 0.67f, 0.9f), buffer.getNumSamples());
    delaySmooth.setTargetValue (fs * *delayTimeMsParam * 0.001f);
    freqSmooth.setTargetValue (*freqParam);

    const auto delayTypeIndex = (int) *delayTypeParam;
    if (delayTypeIndex != prevDelayTypeIndex)
    {
        cleanDelayLine.reset();
        lofiDelayLine.reset();

        prevDelayTypeIndex = delayTypeIndex;
    }

    if (*pingPongParam == 0.0f)
    {
        if (delayTypeIndex == 0)
            processMonoStereoDelay (buffer, cleanDelayLine);
        else if (delayTypeIndex == 1)
            processMonoStereoDelay (buffer, lofiDelayLine);
    }
    else
    {
        if (delayTypeIndex == 0)
            processPingPongDelay (buffer, cleanDelayLine);
        else if (delayTypeIndex == 1)
            processPingPongDelay (buffer, lofiDelayLine);
    }

    bypassNeedsReset = true;
}

void DelayModule::processAudioBypassed (AudioBuffer<float>& buffer)
{
    if (bypassNeedsReset)
    {
        cleanDelayLine.reset();
        lofiDelayLine.reset();
        stereoBuffer.clear();

        bypassNeedsReset = false;
    }

    outputBuffers.getReference (0) = &buffer;
}
