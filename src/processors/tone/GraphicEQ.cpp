#include "GraphicEQ.h"
#include "../ParameterHelpers.h"

namespace
{
const StringArray freqLabels { "100", "220", "500", "1k", "2.2k", "5k" };

/**
 * Q to gain relationship for adaptive Q.
 * The polynomial here is derived in sim/GraphicEQ/adaptive_q.py
 */
constexpr float calcQ (float gainDB)
{
    constexpr float adaptiveQCoeffs[] = { -7.75358366e-09f, 5.21182270e-23f, 2.70080663e-06f, -3.04753193e-20f, -3.29851878e-04f, 1.89860352e-18f, 2.59076683e-02f, -4.77485061e-17f, 3.78416236e-01f };
    return chowdsp::Polynomials::estrin<8> (adaptiveQCoeffs, gainDB);
}
} // namespace

GraphicEQ::GraphicEQ (UndoManager* um) : BaseProcessor ("Graphic EQ", createParameterLayout(), um)
{
    for (int i = 0; i < nBands; ++i)
        chowdsp::ParamUtils::loadParameterPointer (gainDBParams[i], vts, "gain_" + String (i));

    uiOptions.backgroundColour = Colours::burlywood.brighter (0.1f);
    uiOptions.powerColour = Colours::red.darker (0.1f);
    uiOptions.info.description = "A 5-band graphic EQ, with an adaptive Q characteristic.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout GraphicEQ::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();

    for (int i = 0; i < nBands; ++i)
        createGainDBParameter (params, "gain_" + String (i), freqLabels[i], -12.0f, 12.0f, 0.0f);

    return { params.begin(), params.end() };
}

void GraphicEQ::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    fs = (float) sampleRate;

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < nBands; ++i)
        {
            filter[ch][i].reset();

            gainDBSmooth[ch][i].reset (sampleRate, 0.05);
            gainDBSmooth[ch][i].setCurrentAndTargetValue (*gainDBParams[i]);
        }
    }
}

void GraphicEQ::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        for (int i = 0; i < nBands; ++i)
        {
            gainDBSmooth[ch][i].setTargetValue (*gainDBParams[i]);
            if (gainDBSmooth[ch][i].isSmoothing())
            {
                for (int n = 0; n < numSamples; ++n)
                {
                    auto curGainDB = gainDBSmooth[ch][i].getNextValue();
                    auto curQ = calcQ (curGainDB);
                    filter[ch][i].calcCoefsDB (bandFreqs[i], curQ, curGainDB, fs);
                    x[n] = filter[ch][i].processSample (x[n]);
                }
            }
            else
            {
                auto curGainDB = gainDBSmooth[ch][i].getNextValue();
                auto curQ = calcQ (curGainDB);
                filter[ch][i].calcCoefsDB (bandFreqs[i], curQ, curGainDB, fs);
                filter[ch][i].processBlock (x, numSamples);
            }
        }
    }
}
