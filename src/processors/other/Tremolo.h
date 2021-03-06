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

    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* waveParam = nullptr;
    std::atomic<float>* depthParam = nullptr;

    chowdsp::SVFLowpass<float> filter;
    chowdsp::SineWave<float> sine;

    AudioBuffer<float> waveBuffer;
    SmoothedValue<float, ValueSmoothingTypes::Linear> phaseSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> waveSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> depthGainSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> depthAddSmooth;

    float fs = 48000.0f;
    float phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tremolo)
};
