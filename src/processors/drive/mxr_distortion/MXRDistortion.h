#pragma once

#include "../../utility/DCBlocker.h"
#include "MXRDistWDF.h"

class MXRDistortion : public BaseProcessor
{
public:
    explicit MXRDistortion (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* distParam = nullptr;
    std::atomic<float>* levelParam = nullptr;

    MXRDistWDF wdf[2];

    dsp::Gain<float> gain;
    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MXRDistortion)
};
