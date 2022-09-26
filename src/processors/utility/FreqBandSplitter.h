#pragma once

#include "../BaseProcessor.h"

class FreqBandSplitter : public BaseProcessor
{
public:
    explicit FreqBandSplitter (UndoManager* um);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    enum OutputPort
    {
        HighBand,
        MidBand,
        LowBand,
    };
    static constexpr int numOuts = magic_enum::enum_count<OutputPort>();

    chowdsp::FloatParameter* crossLowParam = nullptr;
    chowdsp::FloatParameter* crossHighParam = nullptr;

    chowdsp::SVFLowpass<float> lowCrossLPF1;
    chowdsp::SVFLowpass<float> lowCrossLPF2;
    chowdsp::SVFHighpass<float> lowCrossHPF1;
    chowdsp::SVFHighpass<float> lowCrossHPF2;

    chowdsp::SVFLowpass<float> highCrossLPF1;
    chowdsp::SVFLowpass<float> highCrossLPF2;
    chowdsp::SVFHighpass<float> highCrossHPF1;
    chowdsp::SVFHighpass<float> highCrossHPF2;

    std::array<AudioBuffer<float>, numOuts> buffers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqBandSplitter)
};
