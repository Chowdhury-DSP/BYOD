#pragma once

#include "DryWetProcessor.h"

namespace GlobalParamTags
{
const String monoModeTag = "mono_mode";
const String inGainTag = "in_gain";
const String outGainTag = "out_gain";
const String dryWetTag = "dry_wet";
} // namespace GlobalParamTags

class ChainIOProcessor
{
public:
    explicit ChainIOProcessor (AudioProcessorValueTreeState& vts, std::function<void (int)>&& latencyChangedCallback);

    static void createParameters (Parameters& params);
    void prepare (double sampleRate, int samplesPerBlock);

    int getOversamplingFactor() const;
    dsp::AudioBlock<float> processAudioInput (const AudioBuffer<float>& buffer, bool& sampleRateChanged);
    void processAudioOutput (const AudioBuffer<float>& processedBuffer, AudioBuffer<float>& outputBuffer);

    auto& getOversampling() { return oversampling; }

private:
    bool processChannelInputs (const AudioBuffer<float>& buffer);
    void processChannelOutputs (AudioBuffer<float>& buffer, int numChannelsProcessed) const;

    const std::function<void (int)> latencyChangedCallbackFunc;

    chowdsp::VariableOversampling<float> oversampling;

    std::atomic<float>* monoModeParam = nullptr;
    AudioBuffer<float> ioBuffer;
    dsp::AudioBlock<float> processBlock;

    std::atomic<float>* inGainParam = nullptr;
    std::atomic<float>* outGainParam = nullptr;
    dsp::Gain<float> inGain, outGain;

    std::atomic<float>* dryWetParam = nullptr;
    DryWetProcessor dryWetMixer;

    bool isPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChainIOProcessor)
};
