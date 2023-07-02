#pragma once

#include "processors/BaseProcessor.h"

class ParamModulator : public BaseProcessor
{
public:
    explicit ParamModulator (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&) override;

private:
    chowdsp::FloatParameter* unipolarModParam = nullptr;
    chowdsp::FloatParameter* bipolarModParam = nullptr;
    chowdsp::BoolParameter* bipolarModeParam = nullptr;

    chowdsp::SmoothedBufferValue<float> modSmooth;

    AudioBuffer<float> modOutBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamModulator)
};
