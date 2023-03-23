#include "Flapjack.h"
#include "processors/ParameterHelpers.h"

//https://aionfx.com/news/tracing-journal-crowther-hot-cake/
// Mode 1:
// - transistor is effectively bypassed
// - bias voltage = 4.9V
// Mode 2:
// - transistor is _active_
// - bias voltage = 4V

Flapjack::Flapjack (UndoManager* um)
    : BaseProcessor ("Flapjack", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (driveParam, vts, "drive");
    loadParameterPointer (presenceParam, vts, "presence");
    loadParameterPointer (levelParam, vts, "level");

    uiOptions.backgroundColour = Colours::whitesmoke.darker (0.1f);
    uiOptions.powerColour = Colours::red.darker (0.2f);
    uiOptions.info.description = "Virtual analog emulation of \"Hot Cake\" overdrive pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Flapjack::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "drive", "Drive", 0.5f);
    createPercentParameter (params, "presence", "Presence", 0.5f);
    createPercentParameter (params, "level", "Level", 0.5f);

    return { params.begin(), params.end() };
}

void Flapjack::prepare (double sampleRate, int samplesPerBlock)
{
}

void Flapjack::processAudio (AudioBuffer<float>& buffer)
{
}
