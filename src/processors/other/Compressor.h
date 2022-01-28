#pragma once

#include "../BaseProcessor.h"

class Compressor : public BaseProcessor
{
public:
    explicit Compressor (UndoManager* um);
    ~Compressor() override;

    ProcessorType getProcessorType() const override { return Other; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* threshDBParam = nullptr;
    std::atomic<float>* ratioParam = nullptr;
    std::atomic<float>* kneeDBParam = nullptr;
    std::atomic<float>* attackMsParam = nullptr;
    std::atomic<float>* releaseMsParam = nullptr;
    std::atomic<float>* makeupDBParam = nullptr;

    AudioBuffer<float> levelBuffer;
    chowdsp::LevelDetector<float> levelDetector;

    class GainComputer;
    std::unique_ptr<GainComputer> gainComputer;

    dsp::Gain<float> makeupGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Compressor)
};
