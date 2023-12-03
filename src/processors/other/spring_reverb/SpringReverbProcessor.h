#pragma once

#include "../../BaseProcessor.h"

class SpringReverb;
class SpringReverbProcessor : public BaseProcessor
{
public:
    explicit SpringReverbProcessor (UndoManager* um = nullptr);
    ~SpringReverbProcessor() override;

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void releaseMemory() override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* sizeParam = nullptr;
    chowdsp::FloatParameter* decayParam = nullptr;
    chowdsp::FloatParameter* reflectParam = nullptr;
    chowdsp::FloatParameter* spinParam = nullptr;
    chowdsp::FloatParameter* dampParam = nullptr;
    chowdsp::FloatParameter* chaosParam = nullptr;
    chowdsp::FloatParameter* shakeParam = nullptr;
    chowdsp::FloatParameter* mixParam = nullptr;

    std::unique_ptr<SpringReverb> reverb;

    AudioBuffer<float> dryBuffer;
    dsp::Gain<float> wetGain;

    int numChannels = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpringReverbProcessor)
};
