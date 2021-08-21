#include "SpringReverb.h"

void SpringReverb::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    delay.prepare (spec);
    
    dcBlocker.prepare (spec);
    dcBlocker.setCutoffFrequency (40.0f);

    for (auto& apf : ffAPFs)
        apf.prepare (sampleRate);

    for (auto& apf : fbAPFs)
        apf.prepare (sampleRate);

    lpf.prepare (spec);

    reflectionNetwork.prepare (spec);

    chaosSmooth.reset (sampleRate, 0.05);
}

void SpringReverb::setParams (const Params& params, int numSamples)
{
    auto msToSamples = [=] (float ms) {
        return (ms / 1000.0f) * fs;
    };

    constexpr float lowT60 = 0.5f;
    constexpr float highT60 = 5.0f;
    float t60Seconds = 1.0f * lowT60 * std::pow (highT60 / lowT60, params.decay);

    float delaySamples = 1000.0f + std::pow (params.size * 0.199f, 1.0f) * fs;
    chaosSmooth.setTargetValue (rand.nextFloat() * delaySamples * 0.15f);
    delaySamples += std::pow (params.chaos, 2.0f) * chaosSmooth.skip (numSamples);
    delay.setDelay (delaySamples);

    feedbackGain = std::pow (0.001f, delaySamples / (t60Seconds * fs));

    auto apfG = 0.05f + 0.9f * (1.0f - params.spin);
    for (auto& apf : ffAPFs)
        apf.setParams (msToSamples (0.5f + 3.5f * params.size), apfG);
    
    for (auto& apf : fbAPFs)
        apf.setParams (msToSamples (0.5f + 4.0f * params.size), -apfG);

    constexpr float dampFreqLow = 10000.0f;
    constexpr float dampFreqHigh = 18000.0f;
    auto dampFreq = dampFreqLow * std::pow (dampFreqHigh / dampFreqLow, 1.0f - params.damping);
    lpf.setCutoffFrequency (dampFreq);

    reflectionNetwork.setParams (params.size, t60Seconds, params.reflections * 0.5f, params.damping);
}

void SpringReverb::processBlock (AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            auto y = (x[n] - feedbackGain * delay.popSample (ch));
            y = dcBlocker.processSample<chowdsp::StateVariableFilterType::Highpass> (ch, y);

            for (auto& apf : ffAPFs)
                y = apf.processSample (y);

            y -= reflectionNetwork.popSample (ch);
            reflectionNetwork.pushSample (ch, y);

            y = lpf.processSample<chowdsp::StateVariableFilterType::Lowpass> (ch, y);
            x[n] = y;

            for (auto& apf : fbAPFs)
                y = apf.processSample (y);

            delay.pushSample (ch, y);
        }
    }
}