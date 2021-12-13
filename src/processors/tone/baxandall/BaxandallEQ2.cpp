#include "BaxandallEQ2.h"
#include "../../ParameterHelpers.h"

BaxandallEQ2::BaxandallEQ2 (UndoManager* um) : BaseProcessor ("Baxandall EQ 2", createParameterLayout(), um)
{
    bassParam = vts.getRawParameterValue ("bass");
    trebleParam = vts.getRawParameterValue ("treble");

    uiOptions.backgroundColour = Colours::seagreen;
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "An EQ filter based on Baxandall EQ circuit.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout BaxandallEQ2::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "bass", "Bass", 0.5f);
    createPercentParameter (params, "treble", "Treble", 0.5f);

    return { params.begin(), params.end() };
}

void BaxandallEQ2::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        wdfCircuit[ch].prepare (sampleRate);

        bassSmooth[ch].reset (sampleRate, 0.05);
        bassSmooth[ch].setCurrentAndTargetValue (*bassParam);

        trebleSmooth[ch].reset (sampleRate, 0.05);
        trebleSmooth[ch].setCurrentAndTargetValue (*trebleParam);
    }
}

void BaxandallEQ2::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        bassSmooth[ch].setTargetValue (*bassParam);
        trebleSmooth[ch].setTargetValue (*trebleParam);

        auto* x = buffer.getWritePointer (ch);
        if (bassSmooth[ch].isSmoothing() || trebleSmooth[ch].isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                wdfCircuit[ch].setParams (bassSmooth[ch].getNextValue(), trebleSmooth[ch].getNextValue());
                x[n] = wdfCircuit[ch].processSample (x[n]);
            }
        }
        else
        {
            wdfCircuit[ch].setParams (bassSmooth[ch].getNextValue(), trebleSmooth[ch].getNextValue());

            for (int n = 0; n < numSamples; ++n)
                x[n] = wdfCircuit[ch].processSample (x[n]);
        }
    }
}
