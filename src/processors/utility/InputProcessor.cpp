#include "InputProcessor.h"
#include "../ParameterHelpers.h"

InputProcessor::InputProcessor (UndoManager* um) : BaseProcessor ("Input", createParameterLayout(), um, 0, 1)
{
    uiOptions.backgroundColour = Colours::orange;
}

ParamLayout InputProcessor::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    return { params.begin(), params.end() };
}

void InputProcessor::prepare (double /*sampleRate*/, int /*samplesPerBlock*/)
{
    resetLevels();
}

void InputProcessor::resetLevels()
{
    std::fill (rmsLevels.begin(), rmsLevels.end(), 0.0f);
}

void InputProcessor::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    rmsLevels[0] = buffer.getMagnitude (0, 0, numSamples);
    rmsLevels[1] = buffer.getMagnitude (1 % numChannels, 0, numSamples);
}

bool InputProcessor::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&)
{
    customComps.add (std::make_unique<LevelMeterComponent> (rmsLevels));
    return false;
}
