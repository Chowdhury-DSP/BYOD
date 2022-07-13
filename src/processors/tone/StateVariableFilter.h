#pragma once

#include "../BaseProcessor.h"

class StateVariableFilter : public BaseProcessor
{
public:
    explicit StateVariableFilter (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    using FreqSmooth = SmoothedValue<float, ValueSmoothingTypes::Multiplicative>;

private:
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* qParam = nullptr;
    std::atomic<float>* modeParam = nullptr;

    FreqSmooth freqSmooth;
    //    chowdsp::StateVariableFilter<float> svf; // @TODO: need a multi-mode version of the filter

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StateVariableFilter)
};
