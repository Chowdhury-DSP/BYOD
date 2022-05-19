#pragma once

#include "../BaseProcessor.h"

class Panner : public BaseProcessor
{
public:
    explicit Panner (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    bool getStereoMode() const noexcept { return isStereoInput.load(); }

private:
    void setPanMode();
    void generateModulationSignal (int numSamples);
    void processMonoInput (const AudioBuffer<float>& buffer);
    void processStereoInput (const AudioBuffer<float>& buffer);
    void processSingleChannelPan (chowdsp::Panner<float>& panner, const AudioBuffer<float>& inBuffer, AudioBuffer<float>& outBuffer, float basePanValue, int inBufferChannel = 0, float modMultiply = 1.0f);

    bool getCustomComponents (OwnedArray<Component>& customComps) override;

    std::atomic<float>* mainPan = nullptr;
    std::atomic<float>* leftPan = nullptr;
    std::atomic<float>* rightPan = nullptr;
    std::atomic<float>* stereoWidth = nullptr;
    std::atomic<float>* modDepth = nullptr;
    std::atomic<float>* modRateHz = nullptr;
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
