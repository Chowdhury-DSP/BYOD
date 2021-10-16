#pragma once

#include "../BaseProcessor.h"

class FreqBandSplitter : public BaseProcessor
{
public:
    FreqBandSplitter (UndoManager* um);

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    static constexpr int numOuts = 3;

    std::atomic<float>* crossLowParam = nullptr;
    std::atomic<float>* crossHighParam = nullptr;

    chowdsp::StateVariableFilter<float> lowCrossLPF1;
    chowdsp::StateVariableFilter<float> lowCrossLPF2;
    chowdsp::StateVariableFilter<float> lowCrossHPF1;
    chowdsp::StateVariableFilter<float> lowCrossHPF2;

    chowdsp::StateVariableFilter<float> highCrossLPF1;
    chowdsp::StateVariableFilter<float> highCrossLPF2;
    chowdsp::StateVariableFilter<float> highCrossHPF1;
    chowdsp::StateVariableFilter<float> highCrossHPF2;

    std::array<AudioBuffer<float>, numOuts> buffers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqBandSplitter)
};
