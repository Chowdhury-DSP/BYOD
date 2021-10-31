#pragma once

#include "../BaseProcessor.h"

class StereoMerger : public BaseProcessor
{
public:
    StereoMerger (UndoManager* um);

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* modeParam = nullptr;

    AudioBuffer<float> stereoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoMerger)
};
