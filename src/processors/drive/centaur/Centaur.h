#pragma once

#include "GainStageProc.h"
#include "InputBufferProcessor.h"
#include "OutputStageProcessor.h"
#include "../../utility/DCBlocker.h"

class Centaur : public BaseProcessor
{
public:
    Centaur (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* levelParam = nullptr;

    InputBufferProcessor inProc[2];
    OutputStageProc outProc[2];
    std::unique_ptr<GainStageProc> gainStageProc;

    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Centaur)
};
