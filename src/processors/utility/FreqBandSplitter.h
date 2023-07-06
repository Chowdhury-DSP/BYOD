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

    String getTooltipForPort (int portIndex, bool isInput) override;

    enum OutputPort
    {
        HighBand,
        MidBand,
        LowBand,
    };
    static constexpr int numOuts = magic_enum::enum_count<OutputPort>();

private:
    chowdsp::FloatParameter* crossLowParam = nullptr;
    chowdsp::FloatParameter* crossHighParam = nullptr;

    chowdsp::ThreeWayCrossoverFilter<float, 4> crossover;

    std::array<AudioBuffer<float>, numOuts> buffers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqBandSplitter)
};
