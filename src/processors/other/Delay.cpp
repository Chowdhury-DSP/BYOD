#include "Delay.h"
#include "../ParameterHelpers.h"

Delay::Delay (UndoManager* um) : BaseProcessor ("Delay", createParameterLayout(), um)
{
    delayTimeMsParam = vts.getRawParameterValue ("time_ms");
    freqParam = vts.getRawParameterValue ("freq");
    feedbackParam = vts.getRawParameterValue ("feedback");
    mixParam = vts.getRawParameterValue ("mix");

    uiOptions.backgroundColour = Colours::cyan.darker (0.1f);
    uiOptions.powerColour = Colours::gold;
    uiOptions.info.description = "A BBD emulation delay effect with feedback.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout Delay::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    auto timeRangeMs = createNormalisableRange (0.1f, 2000.0f, 200.0f);
    params.push_back (std::make_unique<VTSParam> ("time_ms",
                                                  "Delay Time",
                                                  String(),
                                                  timeRangeMs,
                                                  100.0f,
                                                  timeMsValToString,
                                                  stringToTimeMsVal));

    createFreqParameter (params, "freq", "Cutoff", 500.0f, 10000.0f, 4000.0f, 10000.0f);
    createPercentParameter (params, "feedback", "Feedback", 0.0f);
    createPercentParameter (params, "mix", "Mix", 0.5f);

    return { params.begin(), params.end() };
}

void Delay::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    dsp::ProcessSpec stereoSpec { sampleRate, (uint32) samplesPerBlock, 2 };
    dsp::ProcessSpec monoSpec = stereoSpec;
    monoSpec.numChannels = 1;

    delayLine.prepare (stereoSpec);

    dryWetMixer.prepare (stereoSpec);
    dryWetMixerMono.prepare (monoSpec);

    delaySmooth.reset (sampleRate, 0.1);
    freqSmooth.reset (sampleRate, 0.1);
    for (auto& sm : fbSmooth)
        sm.reset (sampleRate, 0.01);
}

void Delay::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    dsp::AudioBlock<float> block { buffer };

    auto& dryWet = numChannels == 1 ? dryWetMixerMono : dryWetMixer;
    dryWet.setWetMixProportion (*mixParam);
    dryWet.pushDrySamples (block);

    auto fbTarget = std::pow (feedbackParam->load() * 0.67f, 0.9f);
    delaySmooth.setTargetValue (fs * *delayTimeMsParam * 0.001f);
    freqSmooth.setTargetValue (*freqParam);

    if (delaySmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            auto* x = buffer.getWritePointer (0);
            for (int n = 0; n < numSamples; ++n)
            {
                delayLine.setDelay (delaySmooth.getNextValue());
                delayLine.setFilterFreq (freqSmooth.getNextValue());

                auto y = delayLine.popSample (0);
                delayLine.pushSample (0, x[n] + y * fbSmooth[0].getNextValue());
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
                delayLine.pushSample (0, xL[n] + y * fbSmooth[0].getNextValue());
                xL[n] = y;

                y = delayLine.popSample (1);
                delayLine.pushSample (1, xR[n] + y * fbSmooth[1].getNextValue());
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
            fbSmooth[ch].setTargetValue (fbTarget);
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < numSamples; ++n)
            {
                auto y = delayLine.popSample (ch);
                delayLine.pushSample (ch, x[n] + y * fbSmooth[ch].getNextValue());
                x[n] = y;
            }
        }
    }

    dryWet.mixWetSamples (block);
}
