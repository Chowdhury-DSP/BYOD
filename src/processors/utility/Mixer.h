#pragma once

#include "../BaseProcessor.h"

class Mixer : public BaseProcessor
{
public:
    explicit Mixer (UndoManager* um);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    static constexpr int numIns = 4;
    std::array<std::atomic<float>*, numIns> gainDBParams { nullptr };
    std::array<dsp::Gain<float>, numIns> gains;

    AudioBuffer<float> monoBuffer;
    AudioBuffer<float> stereoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mixer)
};
