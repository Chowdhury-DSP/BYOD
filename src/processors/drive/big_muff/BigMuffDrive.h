#pragma once

#include "../../BaseProcessor.h"
#include "BigMuffClippingStage.h"

class BigMuffDrive : public BaseProcessor
{
public:
    explicit BigMuffDrive (UndoManager* um);

    ProcessorType getProcessorType() const override { return Drive; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    void processInputStage (AudioBuffer<float>& buffer);

    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* harmParam = nullptr;
    std::atomic<float>* levelParam = nullptr;
    std::atomic<float>* nStagesParam = nullptr;

    chowdsp::FirstOrderLPF<float> inputFilter[2];
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> cutoffSmooth;
    dsp::Gain<float> sustainGain;

    BigMuffClippingStage stages[4];

    chowdsp::FirstOrderHPF<float> dcBlocker[2];
    dsp::Gain<float> outLevel;

    float fs = 48000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigMuffDrive)
};
