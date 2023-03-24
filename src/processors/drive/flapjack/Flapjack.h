#pragma once

#include "FlapjackWDF.h"
#include "processors/BaseProcessor.h"

class Flapjack : public BaseProcessor
{
public:
    explicit Flapjack (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* driveParam = nullptr;
    chowdsp::FloatParameter* presenceParam = nullptr;
    chowdsp::FloatParameter* levelParam = nullptr;

    FlapjackWDF wdf[2];
    chowdsp::Gain<float> level;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Flapjack)
};
