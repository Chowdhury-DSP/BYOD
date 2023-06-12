#pragma once

#include "CryBabyWDF.h"
#include "processors/BaseProcessor.h"

class CryBaby : public BaseProcessor
{
public:
    explicit CryBaby (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* controlFreqParam = nullptr;
    chowdsp::SmoothedBufferValue<float> vr1Smooth;

    CryBabyWDF wdf[2];
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryBaby)
};
