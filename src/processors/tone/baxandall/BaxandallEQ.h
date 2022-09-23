#pragma once

#include "../../BaseProcessor.h"
#include "BaxandallWDF.h"

class BaxandallEQ : public BaseProcessor
{
public:
    explicit BaxandallEQ (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* bassParam = nullptr;
    chowdsp::FloatParameter* trebleParam = nullptr;

    SmoothedValue<float, ValueSmoothingTypes::Linear> bassSmooth[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> trebleSmooth[2];

    BaxandallWDF wdfCircuit[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaxandallEQ)
};
