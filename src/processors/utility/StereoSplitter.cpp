#include "StereoSplitter.h"
#include "../ParameterHelpers.h"

StereoSplitter::StereoSplitter (UndoManager* um) : BaseProcessor ("Stereo Splitter", createParameterLayout(), um, 1, 2)
{
    modeParam = vts.getRawParameterValue ("mode");

    uiOptions.backgroundColour = Colours::lightgrey;
    uiOptions.powerColour = Colours::darkred.brighter (0.2f);
    uiOptions.info.description = "Splits a stereo signal into two mono signals.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout StereoSplitter::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    params.push_back (std::make_unique<AudioParameterChoice> ("mode",
                                                              "Mode",
                                                              StringArray { "Left/Right", "Mid/Side" },
                                                              0));

    return { params.begin(), params.end() };
}

void StereoSplitter::prepare (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (sampleRate);

    for (auto& buffer : buffers)
        buffer.setSize (1, samplesPerBlock);
}

void StereoSplitter::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    bool splitLeftRight = *modeParam == 0.0f;

    for (auto& b : buffers)
        b.setSize (1, numSamples, false, false, true);

    if (numChannels == 1)
    {
        if (splitLeftRight)
        {
            buffers[0].copyFrom (0, 0, buffer, 0, 0, numSamples);
            buffers[1].copyFrom (0, 0, buffer, 0, 0, numSamples);
        }
        else
        {
            buffers[0].copyFrom (0, 0, buffer, 0, 0, numSamples);
            buffers[1].clear (0, numSamples);
        }
    }
    else if (numChannels == 2)
    {
        if (splitLeftRight)
        {
            buffers[0].copyFrom (0, 0, buffer, 0, 0, numSamples);
            buffers[1].copyFrom (0, 0, buffer, 1, 0, numSamples);
        }
        else
        {
            buffers[0].copyFrom (0, 0, buffer, 1, 0, numSamples);
            buffers[1].copyFrom (0, 0, buffer, 1, 0, numSamples);
            buffers[1].applyGain (-1.0f);

            buffers[0].addFrom (0, 0, buffer, 0, 0, numSamples);
            buffers[1].addFrom (0, 0, buffer, 0, 0, numSamples);
        }
    }
    else
    {
        jassertfalse;
    }

    outputBuffers.getReference (0) = &buffers[0];
    outputBuffers.getReference (1) = &buffers[1];
}

void StereoSplitter::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto& b : buffers)
        b.setSize (1, numSamples, false, false, true);

    if (numChannels == 1)
    {
        buffers[0].copyFrom (0, 0, buffer, 0, 0, numSamples);
        buffers[1].copyFrom (0, 0, buffer, 0, 0, numSamples);
    }
    else if (numChannels == 2)
    {
        buffers[0].copyFrom (0, 0, buffer, 0, 0, numSamples);
        buffers[1].copyFrom (0, 0, buffer, 1, 0, numSamples);
    }

    outputBuffers.getReference (0) = &buffers[0];
    outputBuffers.getReference (1) = &buffers[1];
}
