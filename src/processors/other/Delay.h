#pragma once

#include "../BaseProcessor.h"

class Delay : public BaseProcessor
{
public:
    Delay (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* delayTimeMsParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    dsp::DryWetMixer<float> dryWetMixer;
    dsp::DryWetMixer<float> dryWetMixerMono;

    using DelayType = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th>;
    DelayType delayLine { 1 << 18 };
    // using DelayType = chowdsp::BBD::BBDDelayWrapper<16384>;
    // DelayType delayLine;

    SmoothedValue<float, ValueSmoothingTypes::Linear> delaySmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> fbSmooth[2];

    float fs = 48000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Delay)
};
