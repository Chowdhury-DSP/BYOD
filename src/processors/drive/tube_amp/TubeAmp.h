#pragma once

#include "../../utility/DCBlocker.h"
#include "TubeProc.h"
#include "processors/BaseProcessor.h"

class TubeAmp : public BaseProcessor
{
public:
    explicit TubeAmp (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* driveParam = nullptr;

    dsp::Gain<float> gainIn, gainOut;
    AudioBuffer<double> doubleBuffer;
    std::unique_ptr<TubeProc> tube[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeAmp)
};
