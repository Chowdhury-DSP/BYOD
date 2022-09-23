#pragma once

#include "HysteresisProcessing.h"
#include "processors/BaseProcessor.h"

class Hysteresis : public BaseProcessor
{
public:
    explicit Hysteresis (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* satParam = nullptr;
    chowdsp::FloatParameter* driveParam = nullptr;
    chowdsp::FloatParameter* widthParam = nullptr;

    HysteresisProcessing hysteresisProc;
    AudioBuffer<double> doubleBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Hysteresis)
};
