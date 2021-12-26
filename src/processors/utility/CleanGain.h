#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class CleanGain : public BaseProcessor
{
public:
    explicit CleanGain (UndoManager* um = nullptr) : BaseProcessor ("Clean Gain", createParameterLayout(), um)
    {
        gainDBParam = vts.getRawParameterValue ("gain");

        uiOptions.backgroundColour = Colours::black;
        uiOptions.powerColour = Colours::yellow;
        uiOptions.info.description = "Simple linear gain boost.";
        uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    }

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace ParameterHelpers;
        auto params = createBaseParams();
        params.push_back (std::make_unique<VTSParam> ("gain",
                                                      "Gain",
                                                      String(),
                                                      NormalisableRange<float> { -18.0f, 18.0f },
                                                      0.0f,
                                                      &gainValToString,
                                                      &stringToGainVal));

        return { params.begin(), params.end() };
    }

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
        gain.prepare (spec);
        gain.setRampDurationSeconds (0.01);
    }

    void processAudio (AudioBuffer<float>& buffer) override
    {
        gain.setGainDecibels (*gainDBParam);

        dsp::AudioBlock<float> block { buffer };
        dsp::ProcessContextReplacing<float> context { block };
        gain.process (context);
    }

private:
    std::atomic<float>* gainDBParam = nullptr;
    dsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanGain)
};
