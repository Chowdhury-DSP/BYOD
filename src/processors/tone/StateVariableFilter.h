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
    bool getCustomComponents (OwnedArray<Component>& customComps) override;

    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* qParam = nullptr;
    std::atomic<float>* modeParam = nullptr;
    std::atomic<float>* multiModeOnOffParam = nullptr;
    std::atomic<float>* multiModeParam = nullptr;

    FreqSmooth freqSmooth;
    chowdsp::SVFMultiMode<float> svf;

    chowdsp::SmoothedBufferValue<float> modeSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StateVariableFilter)
};
