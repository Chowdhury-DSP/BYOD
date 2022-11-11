#pragma once

#include "processors/BaseProcessor.h"

class ShimmerReverb : public BaseProcessor
{
public:
    explicit ShimmerReverb (UndoManager* um);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::SmoothedBufferValue<float> shiftParam;
    chowdsp::SmoothedBufferValue<float, ValueSmoothingTypes::Multiplicative> sizeParam;
    chowdsp::SmoothedBufferValue<float, ValueSmoothingTypes::Multiplicative> feedbackParam;
    chowdsp::FloatParameter* mixParam = nullptr;

    static constexpr int numFDNChannels = 12;
    struct ShimmerFDNConfig : chowdsp::Reverb::DefaultFDNConfig<float, numFDNChannels>
    {
        using Base = chowdsp::Reverb::DefaultFDNConfig<float, numFDNChannels>;
        void prepare (double sampleRate) override;
        void reset() override;
        static const float* doFeedbackProcess (ShimmerFDNConfig& fdnConfig, const float* data);

        chowdsp::PitchShifter<float, chowdsp::DelayLineInterpolationTypes::Linear> shifter { 1 << 15, 2048 };
        chowdsp::FirstOrderHPF<float> lowCutFilter;
        chowdsp::FirstOrderLPF<float> highCutFilter;
    };

    chowdsp::Reverb::FDN<ShimmerFDNConfig> fdn[2];

    static constexpr int numLFOs = 2;
    chowdsp::SineWave<float> lfos[numLFOs];
    float lfoVals[numLFOs] {};

    dsp::DryWetMixer<float> mixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShimmerReverb)
};
