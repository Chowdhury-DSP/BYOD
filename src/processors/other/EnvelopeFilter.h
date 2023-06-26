#pragma once

#include "../BaseProcessor.h"

class EnvelopeFilter : public BaseProcessor
{
public:
    explicit EnvelopeFilter (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp) override;

private:
    void fillLevelBuffer (AudioBuffer<float>& buffer, bool directControlOn);

    chowdsp::FloatParameter* freqParam = nullptr;
    chowdsp::FloatParameter* resParam = nullptr;
    chowdsp::FloatParameter* senseParam = nullptr;
    chowdsp::FloatParameter* speedParam = nullptr;
    chowdsp::FloatParameter* freqModParam = nullptr;
    std::atomic<float>* filterTypeParam = nullptr;
    std::atomic<float>* directControlParam = nullptr;

    chowdsp::SVFMultiMode<float> filter;

    AudioBuffer<float> levelBuffer;
    chowdsp::LevelDetector<float> level;

    enum InputPort
    {
        AudioInput = 0,
        LevelInput,
    };

    enum OutputPort
    {
        AudioOutput = 0,
        LevelOutput,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeFilter)
};
