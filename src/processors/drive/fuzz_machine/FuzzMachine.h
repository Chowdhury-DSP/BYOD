#pragma once

#include "FuzzFaceNDK.h"
#include "processors/BaseProcessor.h"

class FuzzMachine : public BaseProcessor
{
public:
    explicit FuzzMachine (UndoManager* um);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::SmoothedBufferValue<float> fuzzParam;
    chowdsp::PercentParameter* volumeParam = nullptr;

    FuzzFaceNDK model_ndk;

    using AAFilter = chowdsp::EllipticFilter<4>;
    chowdsp::Upsampler<float, AAFilter> upsampler;
    chowdsp::Downsampler<float, AAFilter, false> downsampler;

    chowdsp::FirstOrderHPF<float> dcBlocker;
    chowdsp::Gain<float> volume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuzzMachine)
};
