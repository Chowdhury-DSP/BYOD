#pragma once

#include "../../BaseProcessor.h"
#include "MetalFaceRNN.h"

class MetalFace : public BaseProcessor
{
public:
    explicit MetalFace (UndoManager* um);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* gainDBParam = nullptr;

    dsp::Gain<float> gain;
    MetalFaceRNN<28> rnn[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MetalFace)
};
