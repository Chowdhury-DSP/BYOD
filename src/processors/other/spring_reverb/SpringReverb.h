#pragma once

#include "ReflectionNetwork.h"
#include "SchroederAllpass.h"

class SpringReverb : public chowdsp::RebufferedProcessor<float>
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

    void setParams (const Params& params);

    int prepareRebuffering (const dsp::ProcessSpec& spec) override;
    void processRebufferedBlock (const chowdsp::BufferView<float>& buffer) override;

private:
    void processDownsampledBuffer (AudioBuffer<float>& buffer);

    using AAFilter = chowdsp::ButterworthFilter<8>;
    chowdsp::Downsampler<float, AAFilter> downsample;
    chowdsp::Upsampler<float, AAFilter> upsample;
    AudioBuffer<float> downsampledBuffer;

    chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd> delay { 1 << 18 };
    float feedbackGain = 0.0f;

    chowdsp::SVFHighpass<float> dcBlocker;

    static constexpr int allpassStages = 16;
    using Vec = xsimd::batch<float>;
    using APFCascade = std::array<SchroederAllpass<Vec, 2>, allpassStages>;
    APFCascade vecAPFs;

    Random rand;
    SmoothedValue<float, ValueSmoothingTypes::Linear> chaosSmooth;

    float z[2] { 0.0f, 0.0f };
    float fs = 48000.0f; // downsampled sample rate
    int blockSize = 256; // downsampled block size

    chowdsp::SVFLowpass<float> lpf;

    ReflectionNetwork reflectionNetwork; // early reflections

    int shakeCounter = -1;
    AudioBuffer<float> shakeBuffer;
    AudioBuffer<float> shortShakeBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpringReverb)
};
