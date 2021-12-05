#pragma once

#include "../../BaseProcessor.h"
#include "BaxandallBass.h"
#include "BaxandallTreble.h"

class BaxandallEQ : public BaseProcessor
{
public:
    explicit BaxandallEQ (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* bassParam = nullptr;
    std::atomic<float>* trebleParam = nullptr;

    SmoothedValue<float, ValueSmoothingTypes::Linear> bassSmooth[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> trebleSmooth[2];

    BaxandallBass bassCircuit[2];
    BaxandallTreble trebleCircuit[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaxandallEQ)
};
