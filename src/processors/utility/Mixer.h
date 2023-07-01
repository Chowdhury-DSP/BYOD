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

    String getTooltipForPort (int portIndex, bool isInput) override;

private:
    enum InputPort
    {
        Channel1,
        Channel2,
        Channel3,
        Channel4,
    };

    static constexpr int numIns = magic_enum::enum_count<InputPort>();
    std::array<chowdsp::FloatParameter*, numIns> gainDBParams { nullptr };
    std::array<dsp::Gain<float>, numIns> gains;

    AudioBuffer<float> monoBuffer;
    AudioBuffer<float> stereoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mixer)
};
