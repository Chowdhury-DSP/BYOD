#pragma once

#include "../BaseProcessor.h"

class GraphicEQ : public BaseProcessor
{
public:
    GraphicEQ (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    static constexpr int nBands = 6;
    std::atomic<float>* gainDBParams[nBands];

    static constexpr std::array<float, nBands> bandFreqs { 100.0f, 220.0f, 500.0f, 1000.0f, 2200.0f, 5000.0f };
    std::array<chowdsp::PeakingFilter<float>, nBands> filter[2];

    std::array<SmoothedValue<float, ValueSmoothingTypes::Linear>, nBands> gainDBSmooth[2];

    float fs = 48000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphicEQ)
};
