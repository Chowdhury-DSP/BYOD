#pragma once

#include "SchultePaserFilters.h"
#include "processors/BaseProcessor.h"

class Phaser8 : public BaseProcessor
{
public:
    explicit Phaser8 (UndoManager* um);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    String getTooltipForPort (int portIndex, bool isInput) override;

private:
    void processModulation (int numSamples);

    chowdsp::FloatParameter* rateHzParam = nullptr;
    chowdsp::SmoothedBufferValue<float> depthParam;
    chowdsp::SmoothedBufferValue<float> feedbackParam;
    chowdsp::SmoothedBufferValue<float> modSmooth;
    chowdsp::SmoothedBufferValue<float> noModSmooth;

    SchultePhaserFilters::FeedbackStage fbStage;
    SchultePhaserFilters::FeedbackStageNoMod fbStageNoMod;
    SchultePhaserFilters::ModStages modStages;
    AudioBuffer<float> modulatedOutBuffer;
    AudioBuffer<float> nonModulatedOutBuffer;

    chowdsp::SineWave<float> sineLFO;
    std::vector<float> modData {};
    chowdsp::LookupTableTransform<float> lfoShaper;

    enum InputPort
    {
        AudioInput = 0,
        ModulationInput,
    };

    enum OutputPort
    {
        AudioOutput = 0,
        Stage1Output,
        ModulationOutput,
    };

    AudioBuffer<float> modOutBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Phaser8)
};
