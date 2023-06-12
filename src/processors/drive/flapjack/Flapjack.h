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
    chowdsp::SmoothedBufferValue<float> driveParam;
    chowdsp::SmoothedBufferValue<float> presenceParam;
    chowdsp::SmoothedBufferValue<float, juce::ValueSmoothingTypes::Multiplicative> lowCutParam;
    chowdsp::FloatParameter* levelParam = nullptr;
    chowdsp::ChoiceParameter* modeParam = nullptr;

    FlapjackWDF wdf[2];
    chowdsp::Gain<float> level;
    chowdsp::FirstOrderHPF<float> dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Flapjack)
};
