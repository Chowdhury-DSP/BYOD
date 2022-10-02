#pragma once

#include "../BaseProcessor.h"

class BlondeTone : public BaseProcessor
{
public:
    explicit BlondeTone (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* bassParam = nullptr;
    chowdsp::FloatParameter* midsParam = nullptr;
    chowdsp::FloatParameter* trebleParam = nullptr;

    enum EQBands
    {
        LevelFilter,
        MidsFilter,
        BassFilter,
        TrebleFilter,
        OutFilter,
    };

    using EQBand = chowdsp::EQ::EQBand<float,
                                       chowdsp::SecondOrderLPF<float>,
                                       chowdsp::SVFBell<>,
                                       chowdsp::FirstOrderLPF<float>>;

    chowdsp::EQ::EQProcessor<float, magic_enum::enum_count<EQBands>(), EQBand> filters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlondeTone)
};
