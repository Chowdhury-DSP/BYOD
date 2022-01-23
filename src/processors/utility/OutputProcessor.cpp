#include "OutputProcessor.h"
#include "../ParameterHelpers.h"

OutputProcessor::OutputProcessor (UndoManager* um) : BaseProcessor ("Output", createParameterLayout(), um, 1, 0)
{
    uiOptions.backgroundColour = Colours::lightskyblue;
}

AudioProcessorValueTreeState::ParameterLayout OutputProcessor::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    return { params.begin(), params.end() };
}

void OutputProcessor::prepare (double /*sampleRate*/, int samplesPerBlock)
{
    stereoBuffer.setSize (2, samplesPerBlock);
    std::fill (rmsLevels.begin(), rmsLevels.end(), 0.0f);
}

void OutputProcessor::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels == 1)
    {
        auto processedData = buffer.getReadPointer (0);
        for (int ch = 0; ch < stereoBuffer.getNumChannels(); ++ch)
            FloatVectorOperations::copy (stereoBuffer.getWritePointer (ch),
                                         processedData,
                                         numSamples);

        outputBuffers.getReference (0) = &stereoBuffer;
    }
    else
    {
        outputBuffers.getReference (0) = &buffer;
    }

    // update meter levels
    {
        const auto& outBuffer = outputBuffers.getReference (0);
        rmsLevels[0] = outBuffer->getRMSLevel (0, 0, numSamples);
        rmsLevels[1] = outBuffer->getRMSLevel (1, 0, numSamples);
    }
}

void OutputProcessor::getCustomComponents (OwnedArray<Component>& customComps)
{
    customComps.add (std::make_unique<LevelMeterComponent> (rmsLevels));
}
