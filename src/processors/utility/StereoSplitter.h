#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class StereoSplitter : public BaseProcessor
{
public:
    StereoSplitter (UndoManager* um) : BaseProcessor ("Stereo Splitter", createParameterLayout(), um, 1, 2)
    {
        uiOptions.backgroundColour = Colours::lightgrey;
        uiOptions.powerColour = Colours::darkred.brighter (0.2f);
        uiOptions.info.description = "Splits a stereo signal into two mono signals.";
        uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    }

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace ParameterHelpers;
        auto params = createBaseParams();

        return { params.begin(), params.end() };
    }

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        ignoreUnused (sampleRate);

        leftBuffer.setSize (1, samplesPerBlock);
        rightBuffer.setSize (1, samplesPerBlock);
    }

    void processAudio (AudioBuffer<float>& buffer) override
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        leftBuffer.setSize (1, numSamples, false, false, true);
        rightBuffer.setSize (1, numSamples, false, false, true);

        if (numChannels == 1)
        {
            leftBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
            rightBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
        }
        else if (numChannels == 2)
        {
            leftBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
            rightBuffer.copyFrom (0, 0, buffer, 1, 0, numSamples);
        }
        else
        {
            jassertfalse;
        }

        outputBuffers.getReference (0) = &leftBuffer;
        outputBuffers.getReference (1) = &rightBuffer;
    }

private:
    AudioBuffer<float> leftBuffer;
    AudioBuffer<float> rightBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoSplitter)
};
