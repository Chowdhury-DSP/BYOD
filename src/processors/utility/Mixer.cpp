#include "Mixer.h"
#include "../ParameterHelpers.h"

Mixer::Mixer (UndoManager* um) : BaseProcessor ("Mixer",
                                                createParameterLayout(),
                                                InputPort {},
                                                BasicOutputPort {},
                                                um)
{
    for (int i = 0; i < numIns; ++i)
        chowdsp::ParamUtils::loadParameterPointer (gainDBParams[i], vts, "gain" + String (i));

    uiOptions.backgroundColour = Colours::darkgrey.brighter (0.2f);
    uiOptions.powerColour = Colours::yellow;
    uiOptions.info.description = "Mixes together four input channels.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Mixer::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    for (int i = 0; i < numIns; ++i)
    {
        createGainDBParameter (params, "gain" + String (i), "Channel " + String (i + 1), -18.0f, 18.0f, 0.0f);
    }

    return { params.begin(), params.end() };
}

void Mixer::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    for (auto& gain : gains)
    {
        gain.prepare (spec);
        gain.setRampDurationSeconds (0.01);
    }

    monoBuffer.setSize (1, samplesPerBlock);
    stereoBuffer.setSize (2, samplesPerBlock);
}

void Mixer::processAudio (AudioBuffer<float>& buffer)
{
    AudioBuffer<float>* outBuffer = nullptr;
    int numInputsProcessed = 0;
    for (int i = 0; i < numIns; ++i)
    {
        gains[i].setGainDecibels (*gainDBParams[i]);

        if (! inputsConnected.contains (i))
            continue;

        numInputsProcessed++;

        auto& inBuffer = getInputBuffer (i);
        int numChannels = inBuffer.getNumChannels();
        int numSamples = inBuffer.getNumSamples();

        if (outBuffer == nullptr)
        {
            // we haven't chosen an output buffer yet, so choose it here!
            outBuffer = numChannels == 2 ? &stereoBuffer : &monoBuffer;
            outBuffer->setSize (numChannels, numSamples, false, false, true);
            outBuffer->clear();
        }
        else if (outBuffer == &monoBuffer && numChannels == 2)
        {
            // we chose the mono buffer, but need a stereo output, so convert here
            outBuffer = &stereoBuffer;
            outBuffer->setSize (numChannels, numSamples, false, false, true);

            for (int ch = 0; ch < 2; ++ch)
                outBuffer->copyFrom (ch, 0, monoBuffer, 0, 0, numSamples);
        }

        dsp::AudioBlock<float> block (inBuffer);
        dsp::ProcessContextReplacing<float> ctx (block);
        gains[i].process (ctx);

        for (int ch = 0; ch < outBuffer->getNumChannels(); ++ch)
        {
            int sourceCh = numChannels == 1 ? 0 : ch;
            outBuffer->addFrom (ch, 0, inBuffer, sourceCh, 0, numSamples);
        }
    }

    if (numInputsProcessed == 0)
    {
        buffer.clear();
        outputBuffers.getReference (0) = &buffer;
    }
    else if (numInputsProcessed == 1)
    {
        buffer.makeCopyOf (*outBuffer, true);
        outputBuffers.getReference (0) = &buffer;
    }
    else
    {
        outputBuffers.getReference (0) = outBuffer;
    }
}

void Mixer::processAudioBypassed (AudioBuffer<float>& buffer)
{
    for (int i = 0; i < numIns; ++i)
    {
        if (inputsConnected.contains (i))
        {
            auto& inBuffer = getInputBuffer (i);
            outputBuffers.getReference (0) = &inBuffer;
            return;
        }
    }

    buffer.clear();
    outputBuffers.getReference (0) = &buffer;
}

String Mixer::getTooltipForPort (int portIndex, bool isInput)
{
    if (! isInput)
        return BaseProcessor::getTooltipForPort (portIndex, isInput);

    switch ((InputPort) portIndex)
    {
        case InputPort::Channel1:
            return "Channel 1 Input";
        case InputPort::Channel2:
            return "Channel 2 Input";
        case InputPort::Channel3:
            return "Channel 3 Input";
        case InputPort::Channel4:
            return "Channel 4 Input";
    }
}
