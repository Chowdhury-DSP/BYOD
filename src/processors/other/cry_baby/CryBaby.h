#pragma once

#include "CryBabyWDF.h"
#include "processors/BaseProcessor.h"

struct CryBabyNDK;
class CryBaby : public BaseProcessor
{
public:
    explicit CryBaby (UndoManager* um = nullptr);
    ~CryBaby() override;

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* controlFreqParam = nullptr;
    chowdsp::SmoothedBufferValue<float> alphaSmooth;

    std::unique_ptr<CryBabyNDK> ndk_model;

    chowdsp::FirstOrderHPF<float> dcBlocker;

    using AAFilter = chowdsp::EllipticFilter<4>;
    chowdsp::Upsampler<float, AAFilter> upsampler;
    chowdsp::Downsampler<float, AAFilter, false> downsampler;
    bool needsOversampling = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryBaby)
};
