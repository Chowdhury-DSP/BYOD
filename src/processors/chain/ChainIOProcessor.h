#pragma once

#include "DryWetProcessor.h"
#include "juce_core/system/juce_PlatformDefs.h"

class ChainIOProcessor
{
public:
    ChainIOProcessor (AudioProcessorValueTreeState& vts);

    static void createParameters (Parameters& params);
    void prepare (double sampleRate, int samplesPerBlock);

    int getOversamplingFactor() const;
    dsp::AudioBlock<float> processAudioInput (AudioBuffer<float>& buffer, bool& sampleRateChanged);
    void processAudioOutput (AudioBuffer<float>& buffer);

private:
    std::atomic<float>* oversamplingParam = nullptr;
    std::unique_ptr<dsp::Oversampling<float>> overSample[5];
    int prevOS = 0;

    std::atomic<float>* inGainParam = nullptr;
    std::atomic<float>* outGainParam = nullptr;
    dsp::Gain<float> inGain, outGain;

    std::atomic<float>* dryWetParam = nullptr;
    DryWetProcessor dryWetMixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChainIOProcessor)
};
