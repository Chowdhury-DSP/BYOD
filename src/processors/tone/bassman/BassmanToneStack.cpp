#include "BassmanToneStack.h"

void BassmanToneStack::prepare (double sr)
{
    Cap1.prepare ((float) sr);
    Cap2.prepare ((float) sr);
    Cap3.prepare ((float) sr);

    pot1Smooth.reset (sr, 0.005);
    pot2Smooth.reset (sr, 0.005);
    pot3Smooth.reset (sr, 0.005);
}

void BassmanToneStack::setSMatrixData()
{
    auto pot1 = pot1Smooth.getNextValue();
    Res1m.setResistanceValue (pot1 * R1);
    Res1p.setResistanceValue ((1.0f - pot1) * R1);

    auto pot2 = pot2Smooth.getNextValue();
    Res2.setResistanceValue (pot2 * R2);

    auto pot3 = pot3Smooth.getNextValue();
    Res3m.setResistanceValue (pot3 * R3);
    Res3p.setResistanceValue ((1.0f - pot3) * R3);


}

void BassmanToneStack::process (float* buffer, const int numSamples) noexcept
{
    bool isSmoothing = pot1Smooth.isSmoothing() || pot2Smooth.isSmoothing() || pot3Smooth.isSmoothing();
    if (isSmoothing)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            setSMatrixData();
            buffer[n] = processSample (buffer[n]);
        }

        return;
    }

    for (int n = 0; n < numSamples; ++n)
        buffer[n] = processSample (buffer[n]);
}

void BassmanToneStack::setParams (float pot1, float pot2, float pot3, bool force)
{
    if (force)
    {
        pot1Smooth.setCurrentAndTargetValue (pot1);
        pot2Smooth.setCurrentAndTargetValue (pot2);
        pot3Smooth.setCurrentAndTargetValue (pot3);

        setSMatrixData();
    }
    else
    {
        pot1Smooth.setTargetValue (pot1);
        pot2Smooth.setTargetValue (pot2);
        pot3Smooth.setTargetValue (pot3);
    }
}
