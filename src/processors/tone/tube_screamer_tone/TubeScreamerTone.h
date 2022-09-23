#pragma once

#include "../../BaseProcessor.h"
#include "TubeScreamerToneWDF.h"

class TubeScreamerTone : public BaseProcessor
{
public:
    explicit TubeScreamerTone (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* toneParam = nullptr;

    LinearSmoothedValue<float> toneSmooth[2];
    TubeScreamerToneWDF wdf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeScreamerTone)
};
