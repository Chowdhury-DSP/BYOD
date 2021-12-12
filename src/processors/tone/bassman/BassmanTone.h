#pragma once

#include "BassmanToneStack.h"
#include "processors/BaseProcessor.h"

class BassmanTone : public BaseProcessor
{
public:
    explicit BassmanTone (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    auto cookParameters() const;

    std::atomic<float>* bassParam = nullptr;
    std::atomic<float>* midParam = nullptr;
    std::atomic<float>* trebleParam = nullptr;
    
    BassmanToneStack wdf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassmanTone)
};
