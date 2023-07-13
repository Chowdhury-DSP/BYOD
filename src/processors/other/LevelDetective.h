#pragma once

#include "../BaseProcessor.h"

class LevelDetective : public BaseProcessor
{
public:
    explicit LevelDetective (UndoManager* um);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:

    enum InputPort
    {
        AudioInput
    };

    enum OutputPort
    {
        LevelOutput
    };

    AudioBuffer<float> levelOutBuffer;
    chowdsp::LevelDetector<float> level;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelDetective)
};
