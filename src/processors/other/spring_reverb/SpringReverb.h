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
    };

    void prepare (double sampleRate, int samplesPerBlock);
    void setParams (const Params& params, int numSamples);
    void processBlock (AudioBuffer<float>& buffer);

private:
    chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd> delay { 1 << 18 };
    float feedbackGain;

    chowdsp::StateVariableFilter<float> dcBlocker;

    static constexpr int allpassStages = 50;
    using APFCascade = std::array<SchroederAllpass<2>, allpassStages>;
    APFCascade ffAPFs;
    APFCascade fbAPFs;

    Random rand;
    SmoothedValue<float, ValueSmoothingTypes::Linear> chaosSmooth;

    float fs = 48000.0f;

    chowdsp::StateVariableFilter<float> lpf;

    ReflectionNetwork reflectionNetwork; // early reflections

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpringReverb)
};
