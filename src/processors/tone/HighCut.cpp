#include "HighCut.h"
#include "../ParameterHelpers.h"

namespace
{
constexpr float freq2Rv2 (float cutoff, float C8, float R3)
{
    return (1.0f / (C8 * cutoff)) - R3;
}
}

HighCut::HighCut (UndoManager* um) : BaseProcessor ("High Cut", createParameterLayout(), um)
{
    cutoffParam = vts.getRawParameterValue ("cutoff");

    uiOptions.backgroundColour = Colour (0xFFFF8B3D);
}

AudioProcessorValueTreeState::ParameterLayout HighCut::createParameterLayout()
{
    using namespace ParameterHelpers;
    Params params;

    createFreqParameter (params, "cutoff", "Cutoff", 200.0f, 20.0e3f, 2000.0f, 5000.0f);

    return { params.begin(), params.end() };
}

void HighCut::prepare (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (samplesPerBlock);
    fs = (float) sampleRate;

    Rv2.reset (sampleRate, 0.01);
    Rv2.setCurrentAndTargetValue (freq2Rv2 (*cutoffParam, C8, R3));
    calcCoefs (Rv2.getCurrentValue());
}

void HighCut::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();

    Rv2.setTargetValue (freq2Rv2 (*cutoffParam, C8, R3));
    auto** x = buffer.getArrayOfWritePointers();

    if (Rv2.isSmoothing())
    {
        if (numChannels == 1)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (Rv2.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
            }
        }
        else if (numChannels == 2)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (Rv2.getNextValue());
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
