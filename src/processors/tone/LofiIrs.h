#pragma once

#include "../BaseProcessor.h"

class LofiIrs : public BaseProcessor, private AudioProcessorValueTreeState::Listener
{
public:
    explicit LofiIrs (UndoManager* um = nullptr);
    ~LofiIrs() override;

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void parameterChanged (const String& parameterID, float newValue) override;

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* gainParam = nullptr;

    using IRType = std::pair<void*, size_t>;
    std::unordered_map<String, IRType> irMap;

    dsp::Convolution convolution;
    dsp::Gain<float> gain;

    float makeupGainDB = 0.0f;
    float fs = 48000.0f;

    dsp::DryWetMixer<float> dryWetMixer;
    dsp::DryWetMixer<float> dryWetMixerMono;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LofiIrs)
};
