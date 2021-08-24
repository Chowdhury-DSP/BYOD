#include "Chorus.h"

namespace
{
constexpr float rate1Low = 0.005f;
constexpr float rate1High = 5.0f;
constexpr float rate2Low = 0.5f;
constexpr float rate2High = 40.0f;

constexpr float delay1Ms = 0.6f;
constexpr float delay2Ms = 0.2f;
}

Chorus::Chorus (UndoManager* um) : BaseProcessor ("Chorus", createParameterLayout(), um)
{
    rateParam = vts.getRawParameterValue ("rate");
    depthParam = vts.getRawParameterValue ("depth");
    fbParam = vts.getRawParameterValue ("feedback");
    mixParam = vts.getRawParameterValue ("mix");

    uiOptions.backgroundColour = Colours::purple.brighter (0.25f);
    uiOptions.powerColour = Colours::yellow.brighter(0.1f);
    uiOptions.info.description = "A chorus effect using modulated delay lines based on a model of bucket-brigade device delays.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout Chorus::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createPercentParameter (params, "rate", "Rate", 0.5f);
    createPercentParameter (params, "depth", "Depth", 0.5f);
    createPercentParameter (params, "feedback", "Feedback", 0.0f);
    createPercentParameter (params, "mix", "Mix", 0.5f);

    return { params.begin(), params.end() };
}

void Chorus::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    dsp::ProcessSpec monoSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    fs = (float) sampleRate;
    
    for (int i = 0; i < 4; ++i)
    {
        delay[i].prepare (spec);

        for (int k = 0; k < 2; ++k)
        {
            sines[k][i].prepare (monoSpec);
            sineSmoothers[k][i].reset (sampleRate, 0.01);
            sineSmoothers[k][i].setCurrentAndTargetValue (*depthParam);
        }
    }

    dryWetMixer.prepare (spec);
    dryWetMixerMono.prepare (monoSpec);
    dcBlocker.prepare (spec);
    dcBlocker.setCutoffFrequency (35.0f);

    for (int ch = 0; ch < 2; ++ch)
    {
        feedbackState[ch] = 0.0f;
        fbSmooth[ch].reset (sampleRate, 0.01);
    }
}

void Chorus::processAudio (AudioBuffer<float>& buffer)
{
    auto slowRate = rate1Low * std::pow (rate1High / rate1Low, *rateParam);
    auto fastRate = rate2Low * std::pow (rate2High / rate2Low, *rateParam);
    for (int i = 0; i < 4; ++i)
    {
        sines[0][i].setFrequency (slowRate);
        sines[1][i].setFrequency (fastRate);
        
        sineSmoothers[0][i].setTargetValue (*depthParam);
        sineSmoothers[1][i].setTargetValue (*depthParam);
    }

    auto& dryWet = buffer.getNumChannels() == 1 ? dryWetMixerMono : dryWetMixer;
    dryWet.setWetMixProportion (*mixParam);
    dryWet.pushDrySamples (dsp::AudioBlock<float> { buffer });

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        fbSmooth[ch].setTargetValue (*fbParam);
        float del1 = delay1Ms * 0.001f;
        float del2 = delay2Ms * 0.001f;
        float del0 = del1 + del2;

        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            x[n] = dsp::FastMathApproximations::tanh (x[n] * 0.75f + feedbackState[ch]);

            float y = 0.0f;
            for (int i = 0; i < 2; ++i)
            {
                int chIdx = 2 * ch;
                float tVal = del0;
                tVal += del1 * sines[i][chIdx].processSample() * sineSmoothers[i][chIdx].getNextValue();
                tVal += del2 * sines[i][chIdx+ 1].processSample() * sineSmoothers[i][chIdx + 1].getNextValue();

                int delayIdx = 2 * ch + i;
                delay[delayIdx].setDelay (tVal);
                delay[delayIdx].pushSample (0, x[n]);
                y += delay[delayIdx].popSample (0);
            }

            feedbackState[ch] = fbSmooth[ch].getNextValue() * y;
            x[n] = dcBlocker.processSample<chowdsp::StateVariableFilterType::Highpass> (ch, y);
        }
    }

    buffer.applyGain (Decibels::decibelsToGain (-8.0f));

    dryWet.mixWetSamples (dsp::AudioBlock<float> { buffer });
}
