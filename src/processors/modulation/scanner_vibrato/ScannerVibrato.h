#pragma once

#include "ScannerVibratoWDF.h"
#include "processors/BaseProcessor.h"

class ScannerVibrato : public BaseProcessor
{
public:
    explicit ScannerVibrato (UndoManager* um);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* rateHzParam = nullptr;
    chowdsp::SmoothedBufferValue<float> depthParam;
    chowdsp::FloatParameter* mixParam = nullptr;
    chowdsp::ChoiceParameter* modeParam = nullptr;
    chowdsp::BoolParameter* stereoParam = nullptr;

    chowdsp::SineWave<float> modSource;
    ScannerVibratoWDF wdf[2];
    dsp::DryWetMixer<float> mixer;

    chowdsp::Buffer<float> modsMixBuffer[2];
    chowdsp::Buffer<float> tapsOutBuffer[2];

    chowdsp::LookupTableTransform<float> tapMixTable[ScannerVibratoWDF::numTaps];

    AudioBuffer<float> modOutBuffer;
    AudioBuffer<float> audioOutBuffer;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScannerVibrato)
};
