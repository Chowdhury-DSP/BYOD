#pragma once

#include "../BaseProcessor.h"

class Gate : public BaseProcessor
{
public:
    explicit Gate (UndoManager* um);
    ~Gate() override;

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* threshDBParam = nullptr;
    std::atomic<float>* attackMsParam = nullptr;
    std::atomic<float>* holdMsParam = nullptr;
    std::atomic<float>* releaseMsParam = nullptr;
    std::atomic<float>* makeupDBParam = nullptr;

    AudioBuffer<float> levelBuffer;

    class GateEnvelope;
    std::unique_ptr<GateEnvelope> gateEnvelope;

    dsp::Gain<float> makeupGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gate)
};
