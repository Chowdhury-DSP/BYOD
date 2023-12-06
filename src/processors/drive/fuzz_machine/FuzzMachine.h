#pragma once

#include "processors/BaseProcessor.h"

#include "processors/drive/neural_utils/ResampledRNNAccelerated.h"

class FuzzMachine : public BaseProcessor
{
public:
    explicit FuzzMachine (UndoManager* um);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    enum class Model
    {
        Mode_1z5 = 1,
        Mode_2 = 2,
    };

    chowdsp::SmoothedBufferValue<float> fuzzParam;
    chowdsp::EnumChoiceParameter<Model>* modelParam = nullptr;
    chowdsp::PercentParameter* volumeParam = nullptr;

    static constexpr int hiddenSize = 24;
    ResampledRNNAccelerated<2, hiddenSize> model_ff_15[2];
    ResampledRNNAccelerated<2, hiddenSize> model_ff_2[2];

    using AAFilter = chowdsp::EllipticFilter<4>;
    chowdsp::Upsampler<float, AAFilter> upsampler;
    chowdsp::Downsampler<float, AAFilter, false> downsampler;

    chowdsp::FirstOrderHPF<float> dcBlocker;
    chowdsp::Gain<float> volume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuzzMachine)
};
