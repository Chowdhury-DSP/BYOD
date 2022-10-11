#pragma once

#include "processors/BaseProcessor.h"
#include "CleanDelayType.h"

class Chorus : public BaseProcessor
{
public:
    explicit Chorus (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    template <typename DelayArrType>
    void processChorus (AudioBuffer<float>& buffer, DelayArrType& delay);
    void processModulation (int numSamples);

    chowdsp::FloatParameter* rateParam = nullptr;
    chowdsp::FloatParameter* depthParam = nullptr;
    chowdsp::FloatParameter* fbParam = nullptr;
    chowdsp::FloatParameter* mixParam = nullptr;
    std::atomic<float>* delayTypeParam = nullptr;

    dsp::DryWetMixer<float> dryWetMixer;

    static constexpr int delaysPerChannel = 2;

    SmoothedValue<float, ValueSmoothingTypes::Linear> slowSmooth[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> fastSmooth[2];

    chowdsp::SineWave<float> slowLFOs[2][delaysPerChannel];
    chowdsp::SineWave<float> fastLFOs[2][delaysPerChannel];

    std::vector<float> slowLFOData[2][delaysPerChannel];
    std::vector<float> fastLFOData[2][delaysPerChannel];
    chowdsp::HilbertFilter<float> hilbertFilter[2];

    template <typename DelayType>
    using DelaySet = std::array<std::array<DelayType, delaysPerChannel>, 2>;
    DelaySet<CleanDelayType> cleanDelay;

    using LofiDelayType = chowdsp::BBD::BBDDelayWrapper<1024>;
    DelaySet<LofiDelayType> lofiDelay;
    int prevDelayTypeIndex = 0;

    chowdsp::SVFLowpass<float> aaFilter;

    float feedbackState[2] { 0.0f, 0.0f };
    SmoothedValue<float, ValueSmoothingTypes::Linear> fbSmooth[2];
    chowdsp::SVFHighpass<float> dcBlocker;

    float fs = 48000.0f;
    AudioBuffer<float> audioOutBuffer;
    AudioBuffer<float> modOutBuffer;

    bool bypassNeedsReset = false;

    enum InputPort
    {
        AudioInput = 0,
        ModulationInput,
    };

    enum OutputPort
    {
        AudioOutput = 0,
        ModulationOutput,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chorus)
};
