#pragma once

#include "processors/BaseProcessor.h"

// Based loosely on the Valve Wizard's U-Boat pedal: http://www.valvewizard.co.uk/uboat.html
class Octaver : public BaseProcessor
{
public:
    explicit Octaver (UndoManager* um);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* trackingParam = nullptr;
    chowdsp::FloatParameter* cutoffParam = nullptr;
    chowdsp::FloatParameter* mixParam = nullptr;
    chowdsp::ChoiceParameter* modeParam = nullptr;

    AudioBuffer<float> dryBuffer;
    AudioBuffer<float> envelopeBuffer;

    chowdsp::NthOrderFilter<float, 4> inputHPF;
    chowdsp::NthOrderFilter<float, 8> inputLPF;
    chowdsp::LevelDetector<float> levelTracker;

    chowdsp::NthOrderFilter<float, 4> outputLPF;

    chowdsp::Gain<float> outputGain;

    struct FreqDivider
    {
        bool y = false;
        bool z = false;

        void reset()
        {
            z = false;
            y = false;
        }

        inline bool process (bool x)
        {
            y = (x && ! z) ? ! y : y;
            z = x;
            return y;
        }
    } divider[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Octaver)
};
