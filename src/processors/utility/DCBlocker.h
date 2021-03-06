#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class DCBlocker : public BaseProcessor
{
public:
    explicit DCBlocker (UndoManager* um = nullptr) : BaseProcessor ("DC Blocker", createParameterLayout(), um)
    {
        freqHzParam = vts.getRawParameterValue ("freq");

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
        params.push_back (std::make_unique<VTSParam> ("freq",
                                                      "Frequency",
                                                      String(),
                                                      NormalisableRange<float> { 5.0f, 50.0f },
                                                      30.0f,
                                                      &freqValToString,
                                                      &stringToFreqVal));

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
    std::atomic<float>* freqHzParam = nullptr;
    chowdsp::SVFHighpass<float> filter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DCBlocker)
};
