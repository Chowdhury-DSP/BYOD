#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class Chorus : public BaseProcessor
{
public:
    Chorus (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* fbParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    dsp::DryWetMixer<float> dryWetMixer;

    static constexpr int delaysPerChannel = 2;

    SmoothedValue<float, ValueSmoothingTypes::Linear> slowSmooth[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> fastSmooth[2];

    chowdsp::SineWave<float> slowLFOs[2][delaysPerChannel];
    chowdsp::SineWave<float> fastLFOs[2][delaysPerChannel];

    using DelayType = chowdsp::BBD::BBDDelayWrapper<1024>;
    std::array<std::array<DelayType, delaysPerChannel>, 2> delay;

    chowdsp::StateVariableFilter<float> aaFilter;

    float feedbackState[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> fbSmooth[2];
    chowdsp::StateVariableFilter<float> dcBlocker;

    float fs = 48000.0f;
    AudioBuffer<float> stereoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chorus)
};
