#include "InputProcessor.h"
#include "../ParameterHelpers.h"

InputProcessor::InputProcessor (UndoManager* um) : BaseProcessor ("Input", createParameterLayout(), um, 0, 1)
{
    monoModeParam = vts.getRawParameterValue ("mono_stereo");

    uiOptions.backgroundColour = Colours::orange;
}

ParamLayout InputProcessor::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    params.push_back (std::make_unique<AudioParameterChoice> ("mono_stereo",
                                                              "Mode",
                                                              StringArray { "Mono", "Stereo", "Left", "Right" },
                                                              0));

    return { params.begin(), params.end() };
}

void InputProcessor::prepare (double /*sampleRate*/, int samplesPerBlock)
{
    monoBuffer.setSize (1, samplesPerBlock);
    std::fill (rmsLevels.begin(), rmsLevels.end(), 0.0f);
}

void InputProcessor::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    const auto useMono = monoModeParam->load() == 0.0f;
    const auto useStereo = monoModeParam->load() == 1.0f;
    const auto useLeft = monoModeParam->load() == 2.0f;
    const auto useRight = monoModeParam->load() == 3.0f;

    // simple case: stereo input!
    if (useStereo)
    {
        outputBuffers.getReference (0) = &buffer;
    }
    else
    {
        // Mono output, but which mono?
        monoBuffer.setSize (1, numSamples, false, false, true);
        if (useMono)
        {
            monoBuffer.clear();
            for (int ch = 0; ch < numChannels; ++ch)
                monoBuffer.addFrom (0, 0, buffer.getReadPointer (ch), numSamples);

            monoBuffer.applyGain (1.0f / (float) numChannels);
        }
        else if (useLeft)
        {
            monoBuffer.copyFrom (0, 0, buffer.getReadPointer (0), numSamples);
        }
        else if (useRight)
        {
            monoBuffer.copyFrom (0, 0, buffer.getReadPointer (1 % numChannels), numSamples);
        }

        outputBuffers.getReference (0) = &monoBuffer;
    }

    // update meter levels
    {
        const auto& outBuffer = outputBuffers.getReference (0);
        rmsLevels[0] = outBuffer->getMagnitude (0, 0, numSamples);
        rmsLevels[1] = outBuffer->getMagnitude (jmin (1, outBuffer->getNumChannels() - 1), 0, numSamples);
    }
}

void InputProcessor::getCustomComponents (OwnedArray<Component>& customComps)
{
    customComps.add (std::make_unique<LevelMeterComponent> (rmsLevels));
}
