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

    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

private:
    bool getCustomComponents (OwnedArray<Component>& customComps) override;

    chowdsp::FloatParameter* freqParam = nullptr;
    chowdsp::FloatParameter* qParam = nullptr;
    std::atomic<float>* modeParam = nullptr;
    std::atomic<float>* multiModeOnOffParam = nullptr;
    chowdsp::FloatParameter* multiModeParam = nullptr;

    FreqSmooth freqSmooth;
    chowdsp::SVFMultiMode<float> svf;

    chowdsp::SmoothedBufferValue<float> modeSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StateVariableFilter)
};
