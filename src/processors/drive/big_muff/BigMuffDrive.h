#pragma once

#include "../../BaseProcessor.h"
#include "BigMuffClippingStage.h"

class BigMuffDrive : public BaseProcessor
{
public:
    explicit BigMuffDrive (UndoManager* um);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    void doPrebuffering();
    void processInputStage (AudioBuffer<float>& buffer);

    chowdsp::FloatParameter* sustainParam = nullptr;
    chowdsp::FloatParameter* harmParam = nullptr;
    chowdsp::FloatParameter* levelParam = nullptr;
    chowdsp::SmoothedBufferValue<float> smoothingParam;
    std::atomic<float>* nStagesParam = nullptr;
    std::atomic<float>* hiQParam = nullptr;

    chowdsp::FirstOrderLPF<float> inputFilter[2];
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> cutoffSmooth;
    dsp::Gain<float> sustainGain;

    BigMuffClippingStage stages[4];
    int prevNumStages = 0;

    chowdsp::FirstOrderHPF<float> dcBlocker[2];
    dsp::Gain<float> outLevel;

    float fs = 48000.0f;
    int maxBlockSize = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigMuffDrive)
};
