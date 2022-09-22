#include "StereoMerger.h"
#include "../ParameterHelpers.h"

StereoMerger::StereoMerger (UndoManager* um) : BaseProcessor ("Stereo Merger", createParameterLayout(), um, 2, 1)
{
    modeParam = vts.getRawParameterValue ("mode");

    uiOptions.backgroundColour = Colours::lightgrey;
    uiOptions.powerColour = Colours::darkred.brighter (0.2f);
    uiOptions.info.description = "Merges two mono signals into a stereo signal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout StereoMerger::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    params.push_back (std::make_unique<AudioParameterChoice> ("mode",
                                                              "Mode",
                                                              StringArray { "Left/Right", "Mid/Side" },
                                                              0));

    return { params.begin(), params.end() };
}

void StereoMerger::prepare (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (sampleRate);

    stereoBuffer.setSize (2, samplesPerBlock);
}

void StereoMerger::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    stereoBuffer.setSize (2, numSamples, false, false, true);
    stereoBuffer.clear();

    bool isInput0Connected = inputsConnected.contains (0);
    bool isInput1Connected = inputsConnected.contains (1);
    bool isLeftRight = *modeParam == 0.0f;

    if (! isInput0Connected && ! isInput1Connected)
    {
        buffer.clear();
        outputBuffers.getReference (0) = &buffer;
        return;
    }

    if (isInput0Connected)
    {
        auto& inBuffer = getInputBuffer (0);
        makeBufferMono (inBuffer);

        stereoBuffer.addFrom (0, 0, inBuffer, 0, 0, numSamples);
        if (! isLeftRight)
            stereoBuffer.addFrom (1, 0, inBuffer, 0, 0, numSamples);
    }

    if (isInput1Connected)
    {
        auto& inBuffer = getInputBuffer (1);
        makeBufferMono (inBuffer);

        if (isLeftRight)
        {
            stereoBuffer.addFrom (1, 0, inBuffer, 0, 0, numSamples);
        }
        else
        {
            stereoBuffer.addFrom (0, 0, inBuffer, 0, 0, numSamples);
            inBuffer.applyGain (-1.0f);
            stereoBuffer.addFrom (1, 0, inBuffer, 0, 0, numSamples);
        }
    }

    if (isInput0Connected && isInput1Connected)
        stereoBuffer.applyGain (0.5f);

    outputBuffers.getReference (0) = &stereoBuffer;
}

void StereoMerger::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    stereoBuffer.setSize (2, numSamples, false, false, true);
    stereoBuffer.clear();

    bool isInput0Connected = inputsConnected.contains (0);
    bool isInput1Connected = inputsConnected.contains (1);

    if (! isInput0Connected && ! isInput1Connected)
    {
        buffer.clear();
        outputBuffers.getReference (0) = &buffer;
        return;
    }

    auto& inBuffer = getInputBuffer (isInput0Connected ? 0 : 1);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto sourceCh = inBuffer.getNumChannels() == 1 ? 0 : ch;
        stereoBuffer.addFrom (ch, 0, inBuffer, sourceCh, 0, numSamples);
    }

    outputBuffers.getReference (0) = &stereoBuffer;
}
