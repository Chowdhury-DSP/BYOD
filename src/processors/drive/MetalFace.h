#pragma once

#include "neural_utils/ResampledRNNAccelerated.h"

#include "../BaseProcessor.h"
#include "../utility/DCBlocker.h"

class MetalFace : public BaseProcessor
{
public:
    explicit MetalFace (UndoManager* um);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* gainDBParam = nullptr;

    dsp::Gain<float> gain;
    ResampledRNNAccelerated<1, 28> rnn[2];

    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MetalFace)
};
