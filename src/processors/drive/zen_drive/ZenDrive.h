#pragma once

#include "../../utility/DCBlocker.h"
#include "ZenDriveWDF.h"
#include "processors/BaseProcessor.h"

class ZenDrive : public BaseProcessor
{
public:
    explicit ZenDrive (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* voiceParam = nullptr;
    std::atomic<float>* gainParam = nullptr;

    ZenDriveWDF wdf[2];
    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZenDrive)
};
