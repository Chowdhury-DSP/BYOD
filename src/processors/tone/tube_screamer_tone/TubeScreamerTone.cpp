#include "TubeScreamerTone.h"
#include "../../ParameterHelpers.h"

TubeScreamerTone::TubeScreamerTone (UndoManager* um) : BaseProcessor ("TS-Tone", createParameterLayout(), um)
{
    chowdsp::ParamUtils::loadParameterPointer (toneParam, vts, "tone");

    uiOptions.backgroundColour = Colours::limegreen.darker (0.1f);
    uiOptions.powerColour = Colours::silver.brighter (0.5f);
    uiOptions.info.description = "Virtual analog emulation of the Tube Screamer tone circuit.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout TubeScreamerTone::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "tone", "Tone", 0.5f);

    return { params.begin(), params.end() };
}

void TubeScreamerTone::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    for (auto& w : wdf)
        w.prepare (sampleRate);

    for (auto& toneSm : toneSmooth)
        toneSm.reset (sampleRate, 0.02);
}

void TubeScreamerTone::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        toneSmooth[ch].setTargetValue (*toneParam);
        if (toneSmooth[ch].isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                wdf[ch].setParams (toneSmooth[ch].getNextValue());
                x[n] = wdf[ch].processSample (x[n]);
            }
        }
        else
        {
            wdf[ch].setParams (toneSmooth[ch].getNextValue());
            for (int n = 0; n < numSamples; ++n)
                x[n] = wdf[ch].processSample (x[n]);
        }
    }
}
