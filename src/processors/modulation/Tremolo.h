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

    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

private:
    void fillWaveBuffer_old (float* waveBuff, const int numSamples, float& p);
    void fillWaveBuffer (float* waveBuff, const int numSamples, float& p);

    chowdsp::FloatParameter* rateParam = nullptr;
    chowdsp::FloatParameter* waveParam = nullptr;
    chowdsp::FloatParameter* depthParam = nullptr;
    chowdsp::BoolParameter* stereoParam = nullptr;
    chowdsp::BoolParameter* v1WaveParam = nullptr;

    chowdsp::SVFLowpass<float> filter;

    AudioBuffer<float> modOutBuffer;
    AudioBuffer<float> audioOutBuffer;
    chowdsp::SmoothedBufferValue<float> phaseSmooth;
    chowdsp::SmoothedBufferValue<float> waveSmooth;
    chowdsp::SmoothedBufferValue<float> depthGainSmooth;
    chowdsp::SmoothedBufferValue<float> depthAddSmooth;

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
