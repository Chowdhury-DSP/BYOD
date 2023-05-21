#pragma once

#include "MouseDriveWDF.h"
#include "processors/BaseProcessor.h"

class MouseDrive : public BaseProcessor
{
public:
    explicit MouseDrive (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::SmoothedBufferValue<float, juce::ValueSmoothingTypes::Multiplicative> distortionParam;
    chowdsp::FloatParameter* volumeParam = nullptr;

    MouseDriveWDF wdf[2];
    chowdsp::Gain<float> gain;
    chowdsp::FirstOrderHPF<float> dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MouseDrive)
};
