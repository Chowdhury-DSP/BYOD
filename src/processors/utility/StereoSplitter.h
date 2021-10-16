#pragma once

#include "../BaseProcessor.h"

class StereoSplitter : public BaseProcessor
{
public:
    StereoSplitter (UndoManager* um);

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* modeParam = nullptr;

    AudioBuffer<float> buffers[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoSplitter)
};
