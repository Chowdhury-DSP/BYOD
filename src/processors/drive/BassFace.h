#pragma once

#include "neural_utils/ResampledRNNAccelerated.h"

#include "../BaseProcessor.h"

class BassFace : public BaseProcessor
{
public:
    explicit BassFace (UndoManager* um);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::SmoothedBufferValue<float> gainSmoothed;

    static constexpr int hiddenSize = 24;
    ResampledRNNAccelerated<2, hiddenSize> model[2];
//    using Model = RTNeural::ModelT<float,
//                                   2,
//                                   1,
//                                   RTNeural::LSTMLayerT<float, 2, hiddenSize, RTNeural::SampleRateCorrectionMode::NoInterp>,
//                                   RTNeural::DenseT<float, hiddenSize, 1>>;
//    Model model[2];

    std::unique_ptr<dsp::Oversampling<float>> oversampling;

    chowdsp::SVFHighpass<float> dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassFace)
};
