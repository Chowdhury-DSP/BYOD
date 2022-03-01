#pragma once

#include "../../BaseProcessor.h"
#include "SpringReverb.h"

class SpringReverbProcessor : public BaseProcessor
{
public:
    explicit SpringReverbProcessor (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* sizeParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* reflectParam = nullptr;
    std::atomic<float>* spinParam = nullptr;
    std::atomic<float>* dampParam = nullptr;
    std::atomic<float>* chaosParam = nullptr;
    std::atomic<float>* shakeParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    SpringReverb reverb;

    AudioBuffer<float> dryBuffer;
    dsp::Gain<float> wetGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpringReverbProcessor)
};
