#pragma once

#include "../BaseProcessor.h"

class Rotary : public BaseProcessor
{
public:
    explicit Rotary (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* rateHzParam = nullptr;
    chowdsp::SmoothedBufferValue<float> depthSmoothed;

    void processModulation (int numChannels, int numSamples);
    void processSpectralDelayFilters (int channel, float* data, const float* modData, const float* depthData, int numSamples);

    chowdsp::SineWave<float> modulator;
    AudioBuffer<float> modulationBuffer;

    //    static constexpr int nSpectralFilters = 6;
    //    std::array<float, nSpectralFilters> spectralFilterState[2] {};
    std::vector<float> spectralFilterState[2] {};
    float maxSpectralCoeffMod = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Rotary)
};
