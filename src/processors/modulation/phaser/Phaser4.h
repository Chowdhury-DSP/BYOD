#pragma once

#include "Phase90Filters.h"
#include "processors/BaseProcessor.h"

class Phaser4 : public BaseProcessor
{
public:
    explicit Phaser4 (UndoManager* um);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

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

    static constexpr auto numInputs = (int) magic_enum::enum_count<InputPort>();
    static constexpr auto numOutputs = (int) magic_enum::enum_count<OutputPort>();

private:
    void processModulation (int numSamples);

    chowdsp::FloatParameter* rateHzParam = nullptr;
    chowdsp::SmoothedBufferValue<float> depthParam;
    chowdsp::SmoothedBufferValue<float> feedbackParam;
    chowdsp::ChoiceParameter* fbStageParam = nullptr;
    chowdsp::BoolParameter* stereoParam = nullptr;

    chowdsp::TriangleWave<float> triangleLfo;
    std::vector<float> modData {};
    chowdsp::LookupTableTransform<float> lfoShaper;

    Phase90Filters::Phase90_FB4 fb4Filter[2];
    Phase90Filters::Phase90_FB3 fb3Filter[2];
    Phase90Filters::Phase90_FB2 fb2Filter[2];

    AudioBuffer<float> audioOutBuffer;
    AudioBuffer<float> modOutBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Phaser4)
};
