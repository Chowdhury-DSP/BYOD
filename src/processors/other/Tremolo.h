#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class Tremolo : public BaseProcessor
{
public:
    explicit Tremolo (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    void fillWaveBuffer (float* waveBuff, const int numSamples, float& p);

    chowdsp::FloatParameter* rateParam = nullptr;
    chowdsp::FloatParameter* waveParam = nullptr;
    chowdsp::FloatParameter* depthParam = nullptr;

    chowdsp::SVFLowpass<float> filter;
    chowdsp::SineWave<float> sine;

    AudioBuffer<float> waveBuffer;
    AudioBuffer<float> modBuffer;
    SmoothedValue<float, ValueSmoothingTypes::Linear> phaseSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> waveSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> depthGainSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> depthAddSmooth;

    float fs = 48000.0f;
    float phase = 0.0f;

    enum InputPortIndexes
    {
        AudioInput = 0,
        ModulationInput,
    };

    enum OutputPortIndexes
    {
        AudioOutput = 0,
        ModulationOutput,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tremolo)
};
