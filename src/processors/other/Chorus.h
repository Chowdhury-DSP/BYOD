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
    dsp::DryWetMixer<float> dryWetMixerMono;

    SmoothedValue<float, ValueSmoothingTypes::Linear> sineSmoothers[2][4];
    chowdsp::SineWave<float> sines[2][4];

    chowdsp::BBD::BBDDelayWrapper<2048> delay[4];
    
    chowdsp::StateVariableFilter<float> dcBlocker;

    float feedbackState[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> fbSmooth[2];

    float fs = 48000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chorus)
};
