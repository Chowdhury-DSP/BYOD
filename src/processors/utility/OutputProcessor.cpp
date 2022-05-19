#include "OutputProcessor.h"
#include "../ParameterHelpers.h"

OutputProcessor::OutputProcessor (UndoManager* um) : BaseProcessor ("Output", createParameterLayout(), um, 1, 0)
{
    uiOptions.backgroundColour = Colours::lightskyblue;
}

ParamLayout OutputProcessor::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    return { params.begin(), params.end() };
}

void OutputProcessor::prepare (double /*sampleRate*/, int /*samplesPerBlock*/)
{
    resetLevels();
}

void OutputProcessor::resetLevels()
{
    std::fill (rmsLevels.begin(), rmsLevels.end(), 0.0f);
}

void OutputProcessor::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    rmsLevels[0] = buffer.getMagnitude (0, 0, numSamples);
    rmsLevels[1] = buffer.getMagnitude (1 % numChannels, 0, numSamples);

    outputBuffers.getReference (0) = &buffer;
}

bool OutputProcessor::getCustomComponents (OwnedArray<Component>& customComps)
{
    customComps.add (std::make_unique<LevelMeterComponent> (rmsLevels));
    return false;
}
