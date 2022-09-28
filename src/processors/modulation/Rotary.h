#pragma once

#include "processors/BaseProcessor.h"

class Rotary : public BaseProcessor
{
public:
    explicit Rotary (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* rateHzParam = nullptr;

    void processModulation (int numSamples);
    void processSpectralDelayFilters (int channel, float* data, const float* modData, const float* depthData, int numSamples);
    void processChorusing (int channel, float* data, const float* modData, const float* depthData, int numSamples);

    // modulation
    chowdsp::SineWave<float> modulator;
    AudioBuffer<float> modulationBuffer;
    AudioBuffer<float> modulationBufferNegative;

    // tremolo
    std::vector<float> tremModData;
    chowdsp::SmoothedBufferValue<float> tremDepthSmoothed;

    // spectral filtering
    std::vector<float> spectralFilterState[2] {};
    chowdsp::SmoothedBufferValue<float> spectralDepthSmoothed;

    // chorusing
    struct CleanDelayType
    {
        void prepare (const dsp::ProcessSpec& spec)
        {
            lpf.prepare (spec);
            delay.prepare (spec);
        }

        void reset()
        {
            lpf.reset();
            delay.reset();
        }

        void setDelay (float newDelayInSamples) { delay.setDelay (newDelayInSamples); }
        void setFilterFreq (float freqHz) { lpf.setCutoffFrequency (freqHz); }

        inline void pushSample (int channel, float sample) { delay.pushSample (channel, sample); }
        inline float popSample (int channel) { return lpf.processSample (channel, delay.popSample (channel)); }

        chowdsp::SVFLowpass<float> lpf;
        chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th> delay { 1 << 18 };
    };

    static constexpr int delaysPerChannel = 2;
    using DelaySet = std::array<std::array<CleanDelayType, delaysPerChannel>, 2>;
    DelaySet chorusDelay;
    dsp::DryWetMixer<float> chorusMixer[2];
    float chorusDepthSamples = 0.0f;
    chowdsp::SmoothedBufferValue<float> chorusDepthSmoothed;

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

    AudioBuffer<float> audioOutBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Rotary)
};
