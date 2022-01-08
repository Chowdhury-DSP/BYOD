#pragma once

#include "../BaseProcessor.h"

class EnvelopeFilter : public BaseProcessor
{
public:
    explicit EnvelopeFilter (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    void addToPopupMenu (PopupMenu& menu) override;
    void getCustomComponents (OwnedArray<Component>& customComps) override;

private:
    void fillLevelBuffer (AudioBuffer<float>& buffer, bool directControlOn);

    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* resParam = nullptr;
    std::atomic<float>* senseParam = nullptr;
    std::atomic<float>* speedParam = nullptr;
    std::atomic<float>* freqModParam = nullptr;
    std::atomic<float>* filterTypeParam = nullptr;
    std::atomic<float>* directControlParam = nullptr;

    chowdsp::StateVariableFilter<float> filter;

    AudioBuffer<float> levelBuffer;
    chowdsp::LevelDetector<float> level;

    std::unique_ptr<ParameterAttachment> directControlAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeFilter)
};
