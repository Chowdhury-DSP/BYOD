#pragma once

#include "UniVibeStage.h"
#include "processors/BaseProcessor.h"

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
    chowdsp::FloatParameter* numStagesParam = nullptr;
    chowdsp::BoolParameter* stereoParam = nullptr;
    chowdsp::FloatParameter* mixParam = nullptr;

    chowdsp::SineWave<float> lfo;
    static constexpr size_t maxNumStages = 20;
    std::array<UniVibeStage, maxNumStages> stages;

    dsp::DryWetMixer<float> dryWetMixer;
    dsp::DryWetMixer<float> dryWetMixerMono;

    AudioBuffer<float> modOutBuffer;
    AudioBuffer<float> audioOutBuffer;

    AudioBuffer<float> fadeBuffer;
    int prevNumStages = 1;

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
