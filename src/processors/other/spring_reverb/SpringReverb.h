#pragma once

#include "ReflectionNetwork.h"
#include "SchroederAllpass.h"

class SpringReverb
{
public:
    SpringReverb() = default;

    struct Params
    {
        float size = 0.5f;
        float decay = 0.5f;
        float reflections = 1.0f;
        float spin = 0.5f;
        float damping = 0.5f;
        float chaos = 0.0f;
        bool shake = false;
    };

    void prepare (double sampleRate, int samplesPerBlock);
    void setParams (const Params& params, int numSamples);
    void processBlock (AudioBuffer<float>& buffer);

private:
    chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd> delay { 1 << 18 };
    float feedbackGain;

    chowdsp::StateVariableFilter<float> dcBlocker;

    static constexpr int allpassStages = 16;
    using Vec = dsp::SIMDRegister<float>;
    using APFCascade = std::array<SchroederAllpass<Vec, 2>, allpassStages>;
    APFCascade vecAPFs;

    Random rand;
    SmoothedValue<float, ValueSmoothingTypes::Linear> chaosSmooth;

    float z[2];
    float fs = 48000.0f;
    int maxBlockSize = 256;

    chowdsp::StateVariableFilter<float> lpf;

    ReflectionNetwork reflectionNetwork; // early reflections

    int shakeCounter = -1;
    AudioBuffer<float> shakeBuffer;
    AudioBuffer<float> shortShakeBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpringReverb)
};
