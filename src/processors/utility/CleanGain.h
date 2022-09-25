#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class CleanGain : public BaseProcessor
{
public:
    explicit CleanGain (UndoManager* um = nullptr) : BaseProcessor ("Clean Gain", createParameterLayout(), um)
    {
        chowdsp::ParamUtils::loadParameterPointer (gainDBParam, vts, "gain");
        chowdsp::ParamUtils::loadParameterPointer (invertParam, vts, "invert");

        addPopupMenuParameter ("invert");

        uiOptions.backgroundColour = Colours::darkgrey.brighter (0.35f).withRotatedHue (0.25f);
        uiOptions.powerColour = Colours::yellow;
        uiOptions.info.description = "Simple linear gain boost.";
        uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    }

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout()
    {
        using namespace ParameterHelpers;
        auto params = createBaseParams();
        createGainDBParameter (params, "gain", "Gain", -18.0f, 18.0f, 0.0f);
        emplace_param<chowdsp::BoolParameter> (params, "invert", "Invert", false);

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
        const auto invertGain = invertParam->get() ? -1.0f : 1.0f;
        gain.setGainLinear (Decibels::decibelsToGain (gainDBParam->getCurrentValue()) * invertGain);

        dsp::AudioBlock<float> block { buffer };
        dsp::ProcessContextReplacing<float> context { block };
        gain.process (context);
    }

private:
    chowdsp::FloatParameter* gainDBParam = nullptr;
    chowdsp::BoolParameter* invertParam = nullptr;
    dsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanGain)
};
