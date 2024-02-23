#pragma once

#include "../BaseProcessor.h"

class Oscilloscope : public BaseProcessor
{
public:
    explicit Oscilloscope (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void inputConnectionChanged (int portIndex, bool wasConnected) override;
    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&) override;

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    struct ScopeBackgroundTask : chowdsp::TimeSliceAudioUIBackgroundTask
    {
        ScopeBackgroundTask() : chowdsp::TimeSliceAudioUIBackgroundTask ("Oscilloscope Background Task") {}

        void prepareTask (double sampleRate, int samplesPerBlock, int& requstedBlockSize, int& waitMs) override;
        void resetTask() override;
        void runTask (const AudioBuffer<float>& data) override;

        juce::Point<float> mapXY (int sampleIndex, float yVal) const;
        void setBounds (juce::Rectangle<int> newBounds);
        Path getScopePath() const noexcept;

    private:
        CriticalSection crit;
        Path scopePath;
        juce::Rectangle<float> bounds {};

        int samplesToDisplay = 0;
        int triggerBuffer = 0;
    } scopeTask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscilloscope)
};
