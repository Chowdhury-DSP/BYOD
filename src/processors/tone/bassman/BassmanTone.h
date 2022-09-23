#pragma once

#include "BassmanToneStack.h"
#include "processors/BaseProcessor.h"

class BassmanTone : public BaseProcessor
{
public:
    explicit BassmanTone (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    auto cookParameters() const;

    chowdsp::FloatParameter* bassParam = nullptr;
    chowdsp::FloatParameter* midParam = nullptr;
    chowdsp::FloatParameter* trebleParam = nullptr;

    BassmanToneStack wdf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassmanTone)
};
