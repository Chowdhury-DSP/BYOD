#pragma once

#include "processors/BaseProcessor.h"

class Tremolo : public BaseProcessor
{
public:
    explicit Tremolo (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    void fillWaveBuffer (float* waveBuff, const int numSamples, float& p);

    chowdsp::FloatParameter* rateParam = nullptr;
    chowdsp::FloatParameter* waveParam = nullptr;
    chowdsp::FloatParameter* depthParam = nullptr;
    chowdsp::BoolParameter* stereoParam = nullptr;//this need to be atomic?

    chowdsp::SVFLowpass<float> filter;
    //chowdsp::SineWave<float> sine;//do we need this?

    AudioBuffer<float> modOutBuffer;
    AudioBuffer<float> audioOutBuffer;
    SmoothedValue<float, ValueSmoothingTypes::Linear> phaseSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> waveSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> depthGainSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> depthAddSmooth;

    float fs = 48000.0f;
    float phase = 0.0f;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tremolo)
};
