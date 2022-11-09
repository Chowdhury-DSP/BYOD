#pragma once

#include "../BaseProcessor.h"

class CleanGain : public BaseProcessor
{
public:
    explicit CleanGain (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps) override;

private:
    chowdsp::FloatParameter* gainDBParam = nullptr;
    chowdsp::FloatParameter* extGainDBParam = nullptr;
    chowdsp::BoolParameter* invertParam = nullptr;
    chowdsp::BoolParameter* extendParam = nullptr;

    dsp::Gain<float> gain;

    ParameterAttachment invertAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanGain)
};
