#pragma once

#include "../BaseProcessor.h"
#include "../utility/DCBlocker.h"

class GuitarMLAmp : public BaseProcessor
{
public:
    explicit GuitarMLAmp (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    using ModelType = RTNeural::ModelT<float, 1, 1, RTNeural::LSTMLayerT<float, 1, 20>, RTNeural::DenseT<float, 20, 1>>;

private:
    std::atomic<float>* gainParam = nullptr;
    dsp::Gain<float> inGain;

    std::atomic<float>* modelParam = nullptr;
    std::vector<String> modelTypes;
    std::map<String, ModelType> models[2];

    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarMLAmp)
};
