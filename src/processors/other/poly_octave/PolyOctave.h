#pragma once

#include "DelayPitchShifter.h"
#include "PolyOctaveFilterBankTypes.h"
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

    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

    String getTooltipForPort (int portIndex, bool isInput) override;

    enum OutputPort
    {
        MixOutput,
        Up2Output,
        Up1Output,
        Down1Output,
    };

    static constexpr auto numOutputs = (int) magic_enum::enum_count<OutputPort>();

private:
    void processAudioV1 (AudioBuffer<float>& buffer);

    chowdsp::BoolParameter* v1_mode = nullptr;

    chowdsp::SmoothedBufferValue<float> dryGain {};
    chowdsp::SmoothedBufferValue<float> upOctaveGain {};
    chowdsp::SmoothedBufferValue<float> up2OctaveGain {};
    chowdsp::SmoothedBufferValue<float> downOctaveGain {};

    // V2 processing stuff...
    std::array<poly_octave_v2::ComplexERBFilterBank<poly_octave_v2::N1>, 2> octaveUpFilterBank;
    std::array<poly_octave_v2::ComplexERBFilterBank<poly_octave_v2::N1>, 2> octaveUp2FilterBank;
    std::array<pitch_shift::Processor<float>, 2> downOctavePitchShifters;

    // V1 processing stuff...
    chowdsp::Buffer<double> doubleBuffer;
    chowdsp::Buffer<double> upOctaveBuffer_double;
    chowdsp::Buffer<double> up2OctaveBuffer_double;
    chowdsp::Buffer<double> downOctaveBuffer_double;
    std::array<poly_octave_v1::ComplexERBFilterBank, 2> octaveUpFilterBank_v1;
    std::array<poly_octave_v1::ComplexERBFilterBank, 2> octaveUp2FilterBank_v1;
    std::array<pitch_shift::Processor<double>, 2> downOctavePitchShifters_v1;

    std::array<chowdsp::FirstOrderHPF<float>, (size_t) numOutputs> dcBlocker;

    juce::AudioBuffer<float> mixOutBuffer;
    juce::AudioBuffer<float> up1OutBuffer;
    juce::AudioBuffer<float> up2OutBuffer;
    juce::AudioBuffer<float> down1OutBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolyOctave)
};
