#pragma once

#include "../../BaseProcessor.h"

#include "HighPassLadder.h"
#include "LadderParameters.h"
#include "LowPassLadder.h"

// Low-pass / high-pass filter combination with nonlinear drive modeled after the famous "transistor-ladder"
// Both filters are resonant four-poles, which can self-oscillate
// The models are based on ideas presented by Vadim Zavalishin in his (fantastic) book:
// "The Art of VA Filter Design"
class LadderFilterProcessor : public BaseProcessor
{
public:
    LadderFilterProcessor (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    // Structure handling plugin parameters
    LadderParameters p;

    // The individual ladder filters
    HighPassLadder hp[2];
    LowPassLadder lp[2];

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LadderFilterProcessor)
};