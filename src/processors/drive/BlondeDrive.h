#pragma once

#include "../BaseProcessor.h"

class BlondeDrive : public BaseProcessor
{
public:
    explicit BlondeDrive (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::SmoothedBufferValue<double> driveParamSmooth;
    chowdsp::FloatParameter* characterParam = nullptr;
    chowdsp::SmoothedBufferValue<float> biasParamSmooth;
    chowdsp::BoolParameter* hiQParam = nullptr;

    xsimd::batch<double> state {};

    AudioBuffer<double> doubleBuffer;

    chowdsp::SVFBell<> characterFilter;
    chowdsp::FirstOrderHPF<float> dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlondeDrive)
};
