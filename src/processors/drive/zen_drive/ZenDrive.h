#pragma once

#include "processors/BaseProcessor.h"
#include "ZenDriveWDF.h"
#include "../../utility/DCBlocker.h"

class ZenDrive : public BaseProcessor
{
public:
    ZenDrive (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* voiceParam = nullptr;
    std::atomic<float>* gainParam = nullptr;

    std::unique_ptr<ZenDriveWDF> wdf[2];
    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZenDrive)
};
