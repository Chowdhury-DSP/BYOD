#include "BassCleaner.h"
#include "../ParameterHelpers.h"

BassCleaner::BassCleaner (UndoManager* um) : BaseProcessor ("Bass Cleaner", createParameterLayout(), um)
{
    cleanParam = vts.getRawParameterValue ("clean");

    uiOptions.backgroundColour = Colours::dodgerblue.darker();
    uiOptions.powerColour = Colours::red.brighter (0.1f);
    uiOptions.info.description = "A filter to smooth and dampen bass frequencies.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout BassCleaner::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "clean", "Clean", 0.5f);

    return { params.begin(), params.end() };
}

void BassCleaner::prepare (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (samplesPerBlock);
    fs = (float) sampleRate;

    Rv1.reset (sampleRate, 0.01);
    Rv1.setCurrentAndTargetValue (ParameterHelpers::logPot (*cleanParam) * Rv1Value);
    calcCoefs (Rv1.getCurrentValue());
}

void BassCleaner::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();

    Rv1.setTargetValue (ParameterHelpers::logPot (*cleanParam) * Rv1Value);
    auto** x = buffer.getArrayOfWritePointers();

    if (Rv1.isSmoothing())
    {
        if (numChannels == 1)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (Rv1.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
            }
        }
        else if (numChannels == 2)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (Rv1.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
                x[1][n] = iir[1].processSample (x[1][n]);
            }
        }
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                x[ch][n] = iir[ch].processSample (x[ch][n]);
        }
    }
}
