#pragma once

#include "../../utility/DCBlocker.h"
#include "GainStageML.h"
#include "GainStageProc.h"
#include "InputBufferProcessor.h"
#include "OutputStageProcessor.h"

class Centaur : public BaseProcessor
{
public:
    explicit Centaur (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* levelParam = nullptr;
    std::atomic<float>* modeParam = nullptr;

    InputBufferProcessor inProc[2];
    OutputStageProc outProc[2];
    std::unique_ptr<GainStageProc> gainStageProc;
    GainStageML gainStageML;

    bool useMLPrev = false;
    AudioBuffer<float> fadeBuffer;

    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Centaur)
};
