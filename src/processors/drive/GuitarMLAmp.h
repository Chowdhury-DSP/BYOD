#pragma once

#include "../BaseProcessor.h"
#include "../utility/DCBlocker.h"
#include "neural_utils/ResampledRNN.h"

class GuitarMLAmp : public BaseProcessor
{
public:
    explicit GuitarMLAmp (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* gainParam = nullptr;
    dsp::Gain<float> inGain;

    std::atomic<float>* modelParam = nullptr;
    std::vector<String> modelTypes;

    using ModelType = ResampledRNN<20>;
    std::map<String, ModelType> models[2];

    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarMLAmp)
};
