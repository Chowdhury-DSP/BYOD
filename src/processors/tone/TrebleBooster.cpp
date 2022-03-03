#include "TrebleBooster.h"
#include "../ParameterHelpers.h"

TrebleBooster::TrebleBooster (UndoManager* um) : BaseProcessor ("Treble Booster", createParameterLayout(), um)
{
    trebleParam = vts.getRawParameterValue ("boost");

    uiOptions.backgroundColour = Colours::cyan.darker (0.15f);
    uiOptions.powerColour = Colours::red.darker (0.1f);
    uiOptions.info.description = "A treble boosting filter based on the tone circuit in the Klon Centaur distortion pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout TrebleBooster::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "boost", "Boost", 0.25f);

    return { params.begin(), params.end() };
}

void TrebleBooster::prepare (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (samplesPerBlock);
    fs = (float) sampleRate;

    trebleSmooth.reset (sampleRate, 0.01);
    trebleSmooth.setCurrentAndTargetValue (*trebleParam);
    calcCoefs (trebleSmooth.getCurrentValue());

    for (auto& filt : iir)
        filt.reset();
}

void TrebleBooster::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();

    trebleSmooth.setTargetValue (*trebleParam);
    auto** x = buffer.getArrayOfWritePointers();

    if (trebleSmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (trebleSmooth.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
            }
        }
        else if (numChannels == 2)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (trebleSmooth.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
                x[1][n] = iir[1].processSample (x[1][n]);
            }
        }
    }
    else
    {
        calcCoefs (trebleSmooth.getNextValue());
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                x[ch][n] = iir[ch].processSample (x[ch][n]);
        }
    }
}
