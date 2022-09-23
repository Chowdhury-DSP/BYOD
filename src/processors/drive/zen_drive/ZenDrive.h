#pragma once

#include "../../utility/DCBlocker.h"
#include "ZenDriveWDF.h"
#include "processors/BaseProcessor.h"

class ZenDrive : public BaseProcessor
{
public:
    explicit ZenDrive (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* voiceParam = nullptr;
    chowdsp::FloatParameter* gainParam = nullptr;

    ZenDriveWDF wdf[2];
    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZenDrive)
};
