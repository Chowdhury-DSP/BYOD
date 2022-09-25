#include "StereoSplitter.h"
#include "../ParameterHelpers.h"

StereoSplitter::StereoSplitter (UndoManager* um) : BaseProcessor ("Stereo Splitter", createParameterLayout(), um, 1, numOuts)
{
    modeParam = vts.getRawParameterValue ("mode");

    uiOptions.backgroundColour = Colours::lightgrey;
    uiOptions.powerColour = Colours::red.darker (0.05f);
    uiOptions.info.description = "Splits a stereo signal into two mono signals.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout StereoSplitter::createParameterLayout()
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
            buffers[LeftChannel].copyFrom (0, 0, buffer, 0, 0, numSamples);
            buffers[RightChannel].copyFrom (0, 0, buffer, 0, 0, numSamples);
        }
        else
        {
            buffers[LeftChannel].copyFrom (0, 0, buffer, 0, 0, numSamples);
            buffers[RightChannel].clear (0, numSamples);
        }
    }
    else if (numChannels == 2)
    {
        if (splitLeftRight)
        {
            buffers[LeftChannel].copyFrom (0, 0, buffer, 0, 0, numSamples);
            buffers[RightChannel].copyFrom (0, 0, buffer, 1, 0, numSamples);
        }
        else
        {
            buffers[LeftChannel].copyFrom (0, 0, buffer, 1, 0, numSamples);
            buffers[RightChannel].copyFrom (0, 0, buffer, 1, 0, numSamples);
            buffers[RightChannel].applyGain (-1.0f);

            buffers[LeftChannel].addFrom (0, 0, buffer, 0, 0, numSamples);
            buffers[RightChannel].addFrom (0, 0, buffer, 0, 0, numSamples);
        }
    }
    else
    {
        jassertfalse;
    }

    outputBuffers.getReference (LeftChannel) = &buffers[LeftChannel];
    outputBuffers.getReference (RightChannel) = &buffers[RightChannel];
}

void StereoSplitter::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto& b : buffers)
        b.setSize (1, numSamples, false, false, true);

    if (numChannels == 1)
    {
        buffers[LeftChannel].copyFrom (0, 0, buffer, 0, 0, numSamples);
        buffers[RightChannel].copyFrom (0, 0, buffer, 0, 0, numSamples);
    }
    else if (numChannels == 2)
    {
        buffers[LeftChannel].copyFrom (0, 0, buffer, 0, 0, numSamples);
        buffers[RightChannel].copyFrom (0, 0, buffer, 1, 0, numSamples);
    }

    outputBuffers.getReference (LeftChannel) = &buffers[LeftChannel];
    outputBuffers.getReference (RightChannel) = &buffers[RightChannel];
}
