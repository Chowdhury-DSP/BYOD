#pragma once

#include "processors/BaseProcessor.h"

struct CryBabyNDK;
class CryBaby : public BaseProcessor
{
public:
    explicit CryBaby (UndoManager* um = nullptr);
    ~CryBaby() override;

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp) override;

private:
    void processBlockNDK (const chowdsp::BufferView<float>& block, int smootherDivide = 1);

    chowdsp::FloatParameter* controlFreqParam = nullptr;
    chowdsp::FloatParameter* attackParam = nullptr;
    chowdsp::FloatParameter* releaseParam = nullptr;
    chowdsp::BoolParameter* directControlParam = nullptr;
    chowdsp::SmoothedBufferValue<float> alphaSmooth;
    chowdsp::SmoothedBufferValue<float> depthSmooth;

    std::unique_ptr<CryBabyNDK> ndk_model;

    chowdsp::FirstOrderHPF<float> dcBlocker;

    using AAFilter = chowdsp::EllipticFilter<4>;
    chowdsp::Upsampler<float, AAFilter> upsampler;
    chowdsp::Downsampler<float, AAFilter, false> downsampler;
    bool needsOversampling = false;

    chowdsp::LevelDetector<float> levelDetector;

    AudioBuffer<float> audioOutBuffer;
    AudioBuffer<float> levelOutBuffer;

    enum InputPort
    {
        AudioInput = 0,
        LevelInput,
    };

    enum OutputPort
    {
        AudioOutput = 0,
        LevelOutput,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryBaby)
};
