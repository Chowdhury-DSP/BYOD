#pragma once

#include "../BaseProcessor.h"

class Oscilloscope : public BaseProcessor
{
public:
    explicit Oscilloscope (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void getCustomComponents (OwnedArray<Component>& customComps) override;

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    struct ScopeBackgroundTask : chowdsp::AudioUIBackgroundTask
    {
        ScopeBackgroundTask() : chowdsp::AudioUIBackgroundTask ("Oscilloscope Background Task") {}

        void prepareTask (double sampleRate, int samplesPerBlock, int& requstedBlockSize, int& waitMs) override;
        void runTask (const AudioBuffer<float>& data) override;

        void setBounds (Rectangle<int> newBounds);
        Path getScopePath() const noexcept;

    private:
        CriticalSection crit;
        Path scopePath;
        Rectangle<float> bounds {};

        int samplesToDisplay = 0;
        int triggerBuffer = 0;
    } scopeTask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscilloscope)
};
