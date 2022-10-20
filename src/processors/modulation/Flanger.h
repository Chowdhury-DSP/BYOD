#pragma once

#include "CleanDelayType.h"
#include "processors/BaseProcessor.h"

class Flanger : public BaseProcessor
{
public:
    explicit Flanger (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    template <typename DelayArrType>
    void processFlanger (AudioBuffer<float>& buffer, DelayArrType& delay);
    void processModulation (int numSamples);

    chowdsp::FloatParameter* rateParam = nullptr;
    chowdsp::FloatParameter* fbParam = nullptr;
    chowdsp::FloatParameter* delayAmountParam = nullptr;
    chowdsp::FloatParameter* delayOffsetParam = nullptr;
    chowdsp::FloatParameter* mixParam = nullptr;
    std::atomic<float>* delayTypeParam = nullptr;

    dsp::DryWetMixer<float> dryWetMixer;

    static constexpr int delaysPerChannel = 1;

    SmoothedValue<float, ValueSmoothingTypes::Linear> smooth[2];
    chowdsp::SineWave<float> LFOs[2][delaysPerChannel];
    std::vector<float> LFOData[2][delaysPerChannel];
    chowdsp::HilbertFilter<float> hilbertFilter[1];

    template <typename DelayType>
    using DelaySet = std::array<std::array<DelayType, delaysPerChannel>, 2>;
    DelaySet<CleanDelayType> cleanDelay;

    using LofiDelayType = chowdsp::BBD::BBDDelayWrapper<1024>;
    DelaySet<LofiDelayType> lofiDelay;
    int prevDelayTypeIndex = 0;

    chowdsp::SVFLowpass<float> aaFilter;

    float feedbackState[2] { 0.0f, 0.0f };
    SmoothedValue<float, ValueSmoothingTypes::Linear> fbSmooth[2];

    SmoothedValue<float, ValueSmoothingTypes::Linear> delaySmoothSamples[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> delayOffsetSmoothSamples[2];

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Flanger)
};
