#pragma once

#include "HysteresisProcessing.h"
#include "processors/BaseProcessor.h"

class Hysteresis : public BaseProcessor
{
public:
    explicit Hysteresis (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* satParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* widthParam = nullptr;

    HysteresisProcessing hysteresisProc;
    AudioBuffer<double> doubleBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Hysteresis)
};
