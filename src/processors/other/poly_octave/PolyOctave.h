#pragma once

#include "processors/BaseProcessor.h"

class PolyOctave : public BaseProcessor
{
public:
    explicit PolyOctave (UndoManager* um);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    String getTooltipForPort (int portIndex, bool isInput) override;

    struct ComplexERBFilterBank
    {
        static constexpr size_t numFilterBands = 44;
        using float_2 = xsimd::batch<double>;
        std::array<chowdsp::IIRFilter<4, float_2>, numFilterBands / float_2::size> erbFilterReal, erbFilterImag;
    };

    enum OutputPort
    {
        MixOutput,
        Up1Output,
        Up2Output,
    };

    static constexpr auto numOutputs = (int) magic_enum::enum_count<OutputPort>();

private:
    chowdsp::SmoothedBufferValue<double> dryGain {};
    chowdsp::SmoothedBufferValue<double> upOctaveGain {};
    chowdsp::SmoothedBufferValue<double> up2OctaveGain {};

    chowdsp::Buffer<double> doubleBuffer;
    chowdsp::Buffer<double> upOctaveBuffer;
    chowdsp::Buffer<double> up2OctaveBuffer;

    std::array<ComplexERBFilterBank, 2> octaveUpFilterBank;
    std::array<ComplexERBFilterBank, 2> octaveUp2FilterBank;

    std::array<chowdsp::FirstOrderHPF<float>, (size_t) numOutputs> dcBlocker;

    juce::AudioBuffer<float> mixOutBuffer;
    juce::AudioBuffer<float> up1OutBuffer;
    juce::AudioBuffer<float> up2OutBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolyOctave)
};
