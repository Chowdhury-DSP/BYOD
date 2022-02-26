#pragma once

#include "../BaseProcessor.h"
#include "../utility/DCBlocker.h"

class RangeBooster : public BaseProcessor
{
public:
    explicit RangeBooster (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* rangeParam = nullptr;
    std::atomic<float>* boostParam = nullptr;

    float fs = 48000.0f;
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> freqSmooth[2];
    chowdsp::FirstOrderHPF<float> inputFilter[2];

    double veState[2];
    double c3State[2];
    double c3Coefs[2];

    DCBlocker dcBlocker;
    dsp::Gain<float> outGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RangeBooster)
};
