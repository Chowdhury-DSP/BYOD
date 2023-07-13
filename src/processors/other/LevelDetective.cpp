#include "LevelDetective.h"
#include "../ParameterHelpers.h"

LevelDetective::LevelDetective (UndoManager* um) : BaseProcessor ("Level Detective",
                                                                      createParameterLayout(),
                                                                      InputPort {},
                                                                      OutputPort {},
                                                                      um,
    [] (InputPort port)
    {
        return PortType::audio;
    },
    [] (OutputPort port)
    {
        return PortType::level;
    })
{
    uiOptions.backgroundColour = Colours::teal.darker(0.1f);
    uiOptions.powerColour = Colours::gold.darker(0.1f);
    uiOptions.info.description = "Envelope Follower";
    uiOptions.info.authors = StringArray { "Rachel Locke" };
}

ParamLayout LevelDetective::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    return { params.begin(), params.end() };
}

void LevelDetective::prepare (double sampleRate, int samplesPerBlock)
{

}

void LevelDetective::processAudio (AudioBuffer<float>& buffer)
{

}
