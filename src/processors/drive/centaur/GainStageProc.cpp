#include "GainStageProc.h"

using namespace GainStageSpace;

GainStageProc::GainStageProc (AudioProcessorValueTreeState& vts, double sampleRate) : preAmpL (sampleRate),
                                                                                      preAmpR (sampleRate),
                                                                                      clipL (sampleRate),
                                                                                      clipR (sampleRate),
                                                                                      ff2L (sampleRate),
                                                                                      ff2R (sampleRate)
{
    gainParam = vts.getRawParameterValue ("gain");
}

void GainStageProc::reset (double sampleRate, int samplesPerBlock)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        amp[ch].prepare ((float) sampleRate);
        sumAmp[ch].prepare ((float) sampleRate);
    }

    ff1Buff.setSize (2, samplesPerBlock);
    ff2Buff.setSize (2, samplesPerBlock);
}

void GainStageProc::processBlock (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        auto* x1 = ff1Buff.getWritePointer (ch);
        auto* x2 = ff2Buff.getWritePointer (ch);

        // side chain buffers
        FloatVectorOperations::copy (x2, x, numSamples);

        // Gain stage
        preAmp[ch]->setGain (*gainParam);
        for (int n = 0; n < numSamples; ++n)
        {
            x[n] = preAmp[ch]->processSample (x[n]);
            x1[n] = preAmp[ch]->getFF1();
        }

        // amp stage
        amp[ch].setGain (*gainParam);
        amp[ch].processBlock (x, numSamples);
        FloatVectorOperations::clip (x, x, -4.5f, 4.5f, numSamples);

        // clipping stage
        for (int n = 0; n < numSamples; ++n)
            x[n] = clip[ch]->processSample (x[n]);

        // Feed forward network 2
        ff2[ch]->setGain (*gainParam);
        for (int n = 0; n < numSamples; ++n)
            x2[n] = ff2[ch]->processSample (x2[n]);

        // summing amp
        FloatVectorOperations::add (x, x1, numSamples);
        FloatVectorOperations::add (x, x2, numSamples);
        sumAmp[ch].processBlock (x, numSamples);
        FloatVectorOperations::clip (x, x, -13.1f, 11.7f, numSamples);
    }
}
