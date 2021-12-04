#pragma once

#include "SurgeWaveshapers.h"
#include "processors/BaseProcessor.h"

class Waveshaper : public BaseProcessor
{
public:
    Waveshaper (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* shapeParam = nullptr;

    int lastShape = 0;
    SurgeWaveshapers::QuadFilterWaveshaperState wss;

    SmoothedValue<float, ValueSmoothingTypes::Linear> driveSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Waveshaper)
};
