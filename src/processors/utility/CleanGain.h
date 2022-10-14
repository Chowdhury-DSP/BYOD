#pragma once

#include "../BaseProcessor.h"

class CleanGain : public BaseProcessor,
                  private AudioProcessorValueTreeState::Listener
{
public:
    explicit CleanGain (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps) override;

private:
    void parameterChanged (const String& paramID, float newValue) override;

    chowdsp::FloatParameter* gainDBParam = nullptr;
    chowdsp::FloatParameter* extGainDBParam = nullptr;
    chowdsp::BoolParameter* invertParam = nullptr;
    chowdsp::BoolParameter* extendParam = nullptr;

    dsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanGain)
};
