#pragma once

#include "../BaseProcessor.h"

class StateVariableFilter : public BaseProcessor
{
public:
    StateVariableFilter (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* qParam = nullptr;
    std::atomic<float>* modeParam = nullptr;

    chowdsp::StateVariableFilter<float> svf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StateVariableFilter)
};
