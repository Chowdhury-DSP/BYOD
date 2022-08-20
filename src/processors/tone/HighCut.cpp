#include "HighCut.h"
#include "../ParameterHelpers.h"

namespace
{
constexpr float freq2Rv2 (float cutoff, float C8, float R3)
{
    return (1.0f / (MathConstants<float>::twoPi * C8 * cutoff)) - R3;
}
} // namespace

HighCut::HighCut (UndoManager* um) : BaseProcessor ("High Cut", createParameterLayout(), um)
{
    cutoffParam = vts.getRawParameterValue ("cutoff");

    uiOptions.backgroundColour = Colour (0xFFFF8B3D);
    uiOptions.powerColour = Colours::blue;
    uiOptions.info.description = "A simple high-cut filter with adjustable cutoff frequency.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout HighCut::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createFreqParameter (params, "cutoff", "Cutoff", 30.0f, 20.0e3f, 2000.0f, 5000.0f);

    return { params.begin(), params.end() };
}

void HighCut::prepare (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (samplesPerBlock);
    fs = (float) sampleRate;

    Rv2.reset (sampleRate, 0.01);
    Rv2.setCurrentAndTargetValue (freq2Rv2 (*cutoffParam, C8, R3));
    calcCoefs (Rv2.getCurrentValue());

    for (auto& filt : iir)
        filt.reset();
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
        calcCoefs (Rv2.getNextValue());
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                x[ch][n] = iir[ch].processSample (x[ch][n]);
        }
    }
}

void HighCut::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);

    if (version <= chowdsp::Version { "1.0.1" })
    {
        // Up to version 1.0.2, this module had a bug where the cutoff frequency was off by a factor of 2*pi.
        // The bug is fixed now, but we need to make sure we don't break patches from earlier versions.
        auto* freqParam = vts.getParameter ("cutoff");
        const auto v101FreqHz = freqParam->convertFrom0to1 (freqParam->getValue()) / MathConstants<float>::twoPi;
        freqParam->setValueNotifyingHost (freqParam->convertTo0to1 (v101FreqHz));
    }
}
