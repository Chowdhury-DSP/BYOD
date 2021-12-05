#include "GraphicEQ.h"
#include "../ParameterHelpers.h"

namespace
{
const StringArray freqLabels { "100", "220", "500", "1k", "2.2k", "5k" };

/** Q to gain relationship for adaptive Q */
float calcQ (float gainDB)
{
    const auto gain = Decibels::decibelsToGain (gainDB);

    constexpr float adaptiveQCoeffs[] = { -6.328079654e-9f, 2.737518005e-8f, 2.681260813e-6f, -7.001563368e-6f, -4.001063634e-4f, 4.58428215e-4f, 2.66460649e-2f, -6.555768189e-3f, 1.270585387e-1f };
    return chowdsp::Polynomials::estrin<8> (adaptiveQCoeffs, gain);
}
} // namespace

GraphicEQ::GraphicEQ (UndoManager* um) : BaseProcessor ("Graphic EQ", createParameterLayout(), um)
{
    for (int i = 0; i < nBands; ++i)
        gainDBParams[i] = vts.getRawParameterValue ("gain_" + String (i));

    uiOptions.backgroundColour = Colours::burlywood.brighter (0.1f);
    uiOptions.powerColour = Colours::red.darker (0.1f);
    uiOptions.info.description = "A 5-band graphic EQ.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout GraphicEQ::createParameterLayout()
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
