#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class DCBlocker : public BaseProcessor
{
public:
    explicit DCBlocker (UndoManager* um = nullptr) : BaseProcessor ("DC Blocker", createParameterLayout(), um)
    {
        chowdsp::ParamUtils::loadParameterPointer (freqHzParam, vts, "freq");

        uiOptions.backgroundColour = Colours::darkgrey;
        uiOptions.powerColour = Colours::yellow;
        uiOptions.info.description = "A DC blocking filter with adjustable cutoff frequency.";
        uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    }

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout()
    {
        using namespace ParameterHelpers;
        auto params = createBaseParams();
        createFreqParameter (params, "freq", "Frequency", 5.0f, 50.0f, 27.5f, 30.0f);

        return { params.begin(), params.end() };
    }

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
        filter.prepare (spec);
    }

    void processAudio (AudioBuffer<float>& buffer) override
    {
        filter.setCutoffFrequency (*freqHzParam);
        filter.processBlock (buffer);
    }

private:
    chowdsp::FloatParameter* freqHzParam = nullptr;
    chowdsp::SVFHighpass<float> filter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DCBlocker)
};
