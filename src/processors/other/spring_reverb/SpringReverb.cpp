#include "SpringReverb.h"

namespace
{
constexpr float smallShakeSeconds = 0.0005f;
constexpr float largeShakeSeconds = 0.001f;
} // namespace

void SpringReverb::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;
    maxBlockSize = samplesPerBlock;

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

    shakeCounter = -1;
    shakeBuffer.setSize (1, int (fs * largeShakeSeconds * 3.0f) + maxBlockSize);
    shortShakeBuffer.setSize (1, (int) spec.maximumBlockSize);
}

void SpringReverb::setParams (const Params& params, int numSamples)
{
    auto msToSamples = [=] (float ms)
    {
        return (ms / 1000.0f) * fs;
    };

    if (params.shake && shakeCounter < 0) // start shaking
    {
        float shakeAmount = rand.nextFloat();
        float shakeSeconds = smallShakeSeconds + (largeShakeSeconds - smallShakeSeconds) * shakeAmount;
        shakeSeconds *= 1.0f + 0.5f * params.size;
        shakeCounter = int (fs * shakeSeconds);

        shakeBuffer.setSize (1, shakeCounter + maxBlockSize, false, false, true);
        shakeBuffer.clear();
        auto* shakeBufferPtr = shakeBuffer.getWritePointer (0);
        for (int i = 0; i < shakeCounter; ++i)
            shakeBufferPtr[i] = std::sin (MathConstants<float>::twoPi * (float) i / (2.0f * (float) shakeCounter));

        shakeBuffer.applyGain (2.0f);
    }
    else if (! params.shake && shakeCounter == 0) // reset shake for next time
    {
        shakeCounter = -1;
    }

    constexpr float lowT60 = 0.5f;
    constexpr float highT60 = 4.5f;
    const auto decayCorr = 0.7f * (1.0f - params.size * params.size);
    float t60Seconds = lowT60 * std::pow (highT60 / lowT60, 0.95f * params.decay - decayCorr);

    float delaySamples = 1000.0f + std::pow (params.size * 0.099f, 1.0f) * fs;
    chaosSmooth.setTargetValue (rand.nextFloat() * delaySamples * 0.07f);
    delaySamples += std::pow (params.chaos, 3.0f) * chaosSmooth.skip (numSamples);
    delay.setDelay (delaySamples);

    feedbackGain = std::pow (0.001f, delaySamples / (t60Seconds * fs));

    auto apfG = 0.5f - 0.4f * params.spin;
    float apfGVec alignas (16)[4] = { apfG, -apfG, apfG, -apfG };
    for (auto& apf : vecAPFs)
        apf.setParams (msToSamples (0.35f + 3.0f * params.size), Vec::fromRawArray (apfGVec));

    constexpr float dampFreqLow = 4000.0f;
    constexpr float dampFreqHigh = 18000.0f;
    auto dampFreq = dampFreqLow * std::pow (dampFreqHigh / dampFreqLow, 1.0f - params.damping);
    lpf.setCutoffFrequency (dampFreq);

    auto reflSkew = 0.95f * params.reflections * params.reflections;
    reflectionNetwork.setParams (params.size, t60Seconds, reflSkew, params.damping);
}

void SpringReverb::processBlock (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    shortShakeBuffer.clear();
    const auto* shakePtr = shortShakeBuffer.getReadPointer (0);

    // add shakeBuffer
    if (shakeCounter > 0)
    {
        int startSample = shakeBuffer.getNumSamples() - shakeCounter - maxBlockSize;
        shortShakeBuffer.copyFrom (0, 0, shakeBuffer, 0, startSample, numSamples);
        shakeCounter = jmax (shakeCounter - numSamples, 0);
    }

    // SIMD register for parallel allpass filter
    // format:
    //   [0]: y_left (feedforward path output)
    //   [1]: z_left (feedback path)
    //   [2-3]: equivalents for right channel, or zeros
    float simdReg alignas (16)[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    auto doSpringInput = [=, &shakePtr] (int ch, float input, int n) -> float
    {
        auto output = std::tanh (input - feedbackGain * delay.popSample (ch));
        return dcBlocker.processSample<chowdsp::StateVariableFilterType::Highpass> (ch, output) + shakePtr[n];
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
            simdReg[0] = doSpringInput (0, x[n], n);
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
            simdReg[0] = doSpringInput (0, x_L[n], n);
            simdReg[2] = doSpringInput (1, x_R[n], n);

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