#include "BaxandallEQ.h"
#include "../../ParameterHelpers.h"

BaxandallEQ::BaxandallEQ (UndoManager* um) : BaseProcessor ("Baxandall EQ", createParameterLayout(), um)
{
    bassParam = vts.getRawParameterValue ("bass");
    trebleParam = vts.getRawParameterValue ("treble");

    uiOptions.backgroundColour = Colours::seagreen;
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "An EQ filter based on Baxandall EQ circuit.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout BaxandallEQ::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "bass", "Bass", 0.5f);
    createPercentParameter (params, "treble", "Treble", 0.5f);

    return { params.begin(), params.end() };
}

void BaxandallEQ::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        bassCircuit[ch].prepare (sampleRate);
        trebleCircuit[ch].prepare (sampleRate);

        bassSmooth[ch].reset (sampleRate, 0.05);
        bassSmooth[ch].setCurrentAndTargetValue (*bassParam);

        trebleSmooth[ch].reset (sampleRate, 0.05);
        trebleSmooth[ch].setCurrentAndTargetValue (*trebleParam);
    }
}

void BaxandallEQ::processAudio (AudioBuffer<float>& buffer)
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
                bassCircuit[ch].setBass (bassSmooth[ch].getNextValue());
                trebleCircuit[ch].setTreble (trebleSmooth[ch].getNextValue());
                x[n] = bassCircuit[ch].processSample (x[n]) + trebleCircuit[ch].processSample (x[n]);
            }
        }
        else
        {
            bassCircuit[ch].setBass (bassSmooth[ch].getNextValue());
            trebleCircuit[ch].setTreble (trebleSmooth[ch].getNextValue());

            for (int n = 0; n < numSamples; ++n)
                x[n] = bassCircuit[ch].processSample (x[n]) + trebleCircuit[ch].processSample (x[n]);
        }
    }
}
