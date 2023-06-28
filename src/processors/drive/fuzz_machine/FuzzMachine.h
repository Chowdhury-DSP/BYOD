#pragma once

#include "processors/BaseProcessor.h"

#include "FuzzFaceSolver.h"

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

    FuzzFaceSolver model;

    chowdsp::FirstOrderHPF<float> dcBlocker;
    chowdsp::Gain<float> volume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuzzMachine)
};
