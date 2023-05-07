#pragma once

#include "../../utility/DCBlocker.h"
#include "TubeScreamerWDF.h"
#include "processors/BaseProcessor.h"

class TubeScreamer : public BaseProcessor
{
public:
    explicit TubeScreamer (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* gainParam = nullptr;
    std::atomic<float>* diodeTypeParam = nullptr;
    chowdsp::FloatParameter* nDiodesParam = nullptr;

    TubeScreamerWDF wdf[2];
    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeScreamer)
};
