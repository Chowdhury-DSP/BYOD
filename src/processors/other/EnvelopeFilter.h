#pragma once

#include "../BaseProcessor.h"

class EnvelopeFilter : public BaseProcessor
{
public:
    EnvelopeFilter (UndoManager* um = nullptr);
    ~EnvelopeFilter() override;

    ProcessorType getProcessorType() const override { return Other; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* resParam = nullptr;
    std::atomic<float>* senseParam = nullptr;
    std::atomic<float>* speedParam = nullptr;
    std::atomic<float>* filterTypeParam = nullptr;

    chowdsp::StateVariableFilter<float> filter;

    AudioBuffer<float> levelBuffer;
    chowdsp::LevelDetector<float> level;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeFilter)
};
