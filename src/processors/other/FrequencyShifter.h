#pragma once

#include "../BaseProcessor.h"

class FrequencyShifter : public BaseProcessor
{
public:
    explicit FrequencyShifter (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* shiftParam = nullptr;

    chowdsp::HilbertFilter<float> hilbertFilter;
    chowdsp::SineWave<float> modulator {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyShifter)
};
