#pragma once

#include "../BaseProcessor.h"

class Compressor : public BaseProcessor
{
public:
    explicit Compressor (UndoManager* um);
    ~Compressor() override;

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* threshDBParam = nullptr;
    chowdsp::FloatParameter* ratioParam = nullptr;
    chowdsp::FloatParameter* kneeDBParam = nullptr;
    chowdsp::FloatParameter* attackMsParam = nullptr;
    chowdsp::FloatParameter* releaseMsParam = nullptr;
    chowdsp::FloatParameter* makeupDBParam = nullptr;

    AudioBuffer<float> levelBuffer;
    AudioBuffer<float> levelOutBuffer;
    AudioBuffer<float> audioOutBuffer;
    chowdsp::LevelDetector<float> levelDetector;

    class GainComputer;
    std::unique_ptr<GainComputer> gainComputer;

    dsp::Gain<float> makeupGain;

    enum InputPort
    {
        AudioInput = 0,
        LevelInput,
    };

    enum OutputPort
    {
        AudioOutput = 0,
        LevelOutput,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Compressor)
};
