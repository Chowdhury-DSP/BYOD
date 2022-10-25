#pragma once

#include "../BaseProcessor.h"

class SmoothReverb : public BaseProcessor
{
public:
    explicit SmoothReverb (UndoManager* um);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    void processReverb (float* left, float* right, int numSamples);

    chowdsp::FloatParameter* decayMsParam = nullptr;
    chowdsp::FloatParameter* relaxParam = nullptr;
    chowdsp::FloatParameter* lowCutHzParam = nullptr;
    chowdsp::FloatParameter* highCutHzParam = nullptr;
    chowdsp::FloatParameter* mixPctParam = nullptr;

    using Delay = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th>;
    Delay preDelay1 { 1 << 18 };
    Delay preDelay2 { 1 << 18 };

    chowdsp::NthOrderFilter<xsimd::batch<float>, 4> preDelayFilt;

    static constexpr int nDiffuserChannels = 8;
    chowdsp::Reverb::DiffuserChain<4, chowdsp::Reverb::Diffuser<float, nDiffuserChannels>> diffuser;
    static constexpr int nFDNChannels = 12;
    chowdsp::Reverb::FDN<chowdsp::Reverb::DefaultFDNConfig<float, nFDNChannels>> fdn;

    chowdsp::LevelDetector<float> envelopeFollower;

    chowdsp::SVFHighpass<> lowCutFilter;
    chowdsp::NthOrderFilter<float, 4> highCutFilter;
    dsp::DryWetMixer<float> mixer;

    AudioBuffer<float> outBuffer;

    float fs = 480000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmoothReverb)
};
