#pragma once

#include "processors/BaseProcessor.h"
#include "UniVibeStage.h"

class UniVibe : public BaseProcessor
{
public:
    explicit UniVibe (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    chowdsp::SmoothedBufferValue<float, juce::ValueSmoothingTypes::Multiplicative> speedParamSmooth;
    chowdsp::SmoothedBufferValue<float, juce::ValueSmoothingTypes::Linear> intensityParamSmooth;
    chowdsp::FloatParameter* mixParam = nullptr;

    chowdsp::SineWave<float> lfo;
    UniVibeStage stages[4];

    AudioBuffer<float> modOutBuffer;
    AudioBuffer<float> audioOutBuffer;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UniVibe)
};
