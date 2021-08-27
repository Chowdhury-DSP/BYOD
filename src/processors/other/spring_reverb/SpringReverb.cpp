#include "SpringReverb.h"

void SpringReverb::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    delay.prepare (spec);

    dcBlocker.prepare (spec);
    dcBlocker.setCutoffFrequency (40.0f);

    for (auto& apf : vecAPFs)
        apf.prepare (sampleRate);

    lpf.prepare (spec);

    reflectionNetwork.prepare (spec);

    chaosSmooth.reset (sampleRate, 0.05);

    z[0] = 0.0f;
    z[1] = 0.0f;
}

void SpringReverb::setParams (const Params& params, int numSamples)
{
    auto msToSamples = [=] (float ms)
    {
        return (ms / 1000.0f) * fs;
    };

    constexpr float lowT60 = 0.5f;
    constexpr float highT60 = 4.5f;
    float t60Seconds = lowT60 * std::pow (highT60 / lowT60, params.decay - 0.7f * (1.0f - std::pow (params.size, 2.0f)));

    float delaySamples = 1000.0f + std::pow (params.size * 0.099f, 1.0f) * fs;
    chaosSmooth.setTargetValue (rand.nextFloat() * delaySamples * 0.07f);
    delaySamples += std::pow (params.chaos, 3.0f) * chaosSmooth.skip (numSamples);
    delay.setDelay (delaySamples);

    feedbackGain = std::pow (0.001f, delaySamples / (t60Seconds * fs));

    auto apfG = 0.5f - 0.4f * params.spin;
    float apfGVec alignas (16)[4] = { apfG, -apfG, apfG, -apfG };
    for (auto& apf : vecAPFs)
        apf.setParams (msToSamples (0.35f + params.size), Vec::fromRawArray (apfGVec));

    constexpr float dampFreqLow = 4000.0f;
    constexpr float dampFreqHigh = 18000.0f;
    auto dampFreq = dampFreqLow * std::pow (dampFreqHigh / dampFreqLow, 1.0f - params.damping);
    lpf.setCutoffFrequency (dampFreq);

    reflectionNetwork.setParams (params.size, t60Seconds, params.reflections * 0.5f, params.damping);
}

void SpringReverb::processBlock (AudioBuffer<float>& buffer)
{
    // SIMD register for parallel allpass filter
    // format:
    //   [0]: y_left (feedforward path output)
    //   [1]: z_left (feedback path)
    //   [2-3]: equivalents for right channel, or zeros
    float simdReg alignas (16)[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    auto doSpringInput = [=] (int ch, float input) -> float
    {
        auto output = std::tanh (input - feedbackGain * delay.popSample (ch));
        return dcBlocker.processSample<chowdsp::StateVariableFilterType::Highpass> (ch, output);
    };

    auto doAPFProcess = [&]()
    {
        auto yVec = Vec::fromRawArray (simdReg);
        for (auto& apf : vecAPFs)
            yVec = apf.processSample (yVec);
        yVec.copyToRawArray (simdReg);
    };

    auto doSpringOutput = [&] (int ch)
    {
        auto chIdx = 2 * ch;
        delay.pushSample (ch, simdReg[1 + chIdx]);

        simdReg[chIdx] -= reflectionNetwork.popSample (ch);
        reflectionNetwork.pushSample (ch, simdReg[chIdx]);
        simdReg[chIdx] = lpf.processSample<chowdsp::StateVariableFilterType::Lowpass> (ch, simdReg[chIdx]);
    };

    if (numChannels == 1)
    {
        simdReg[1] = z[0];

        auto* x = buffer.getWritePointer (0);
        for (int n = 0; n < numSamples; ++n)
        {
            simdReg[0] = doSpringInput (0, x[n]);
            doAPFProcess();
            doSpringOutput (0);

            x[n] = simdReg[0]; // write to output
            simdReg[1] = simdReg[0]; // write to feedback path
        }

        z[0] = simdReg[1];
    }
    else if (numChannels == 2)
    {
        simdReg[1] = z[0];
        simdReg[3] = z[1];

        auto* x_L = buffer.getWritePointer (0);
        auto* x_R = buffer.getWritePointer (1);
        for (int n = 0; n < numSamples; ++n)
        {
            simdReg[0] = doSpringInput (0, x_L[n]);
            simdReg[2] = doSpringInput (1, x_R[n]);

            doAPFProcess();

            doSpringOutput (0);
            doSpringOutput (1);

            x_L[n] = simdReg[0]; // write to output
            simdReg[1] = simdReg[0]; // write to feedback path
            x_R[n] = simdReg[2];
            simdReg[3] = simdReg[2];
        }

        z[0] = simdReg[1];
        z[1] = simdReg[3];
    }
}