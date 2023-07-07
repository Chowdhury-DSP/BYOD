#pragma once

#include "processors/BaseProcessor.h"

class Panner : public BaseProcessor
{
public:
    explicit Panner (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    bool getStereoMode() const noexcept { return isStereoInput.load(); }

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

    static constexpr auto numInputs = (int) magic_enum::enum_count<InputPort>();
    static constexpr auto numOutputs = (int) magic_enum::enum_count<OutputPort>();

private:
    void setPanMode();
    void generateModulationSignal (int numSamples);
    void processMonoInput (const AudioBuffer<float>& buffer);
    void processStereoInput (const AudioBuffer<float>& buffer);
    void processSingleChannelPan (chowdsp::Panner<float>& panner, const AudioBuffer<float>& inBuffer, AudioBuffer<float>& outBuffer, float basePanValue, int inBufferChannel = 0, float modMultiply = 1.0f);

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp) override;

    chowdsp::FloatParameter* mainPan = nullptr;
    chowdsp::FloatParameter* leftPan = nullptr;
    chowdsp::FloatParameter* rightPan = nullptr;
    chowdsp::FloatParameter* stereoWidth = nullptr;
    chowdsp::FloatParameter* modDepth = nullptr;
    chowdsp::FloatParameter* modRateHz = nullptr;
    std::atomic<float>* panMode = nullptr;
    std::atomic<float>* stereoMode = nullptr;

    chowdsp::Panner<float> panners[2];
    AudioBuffer<float> stereoBuffer, tempStereoBuffer;

    chowdsp::SineWave<float> modulator;
    dsp::Gain<float> modulationGain;
    AudioBuffer<float> modulationBuffer;
    bool isModulationOn = true;

    std::atomic_bool isStereoInput { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Panner)
};
