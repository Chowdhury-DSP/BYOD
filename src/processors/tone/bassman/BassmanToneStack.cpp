#include "BassmanToneStack.h"

void BassmanToneStack::prepare (double sr)
{
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

    const auto Ra = S1.R;
    const auto Rb = S3.R;
    const auto Rc = S2.R;
    const auto Rd = Cap2.R;
    const auto Re = Res4.R;
    const auto Rf = Cap3.R;
    const auto Ga = 1.0f / Ra;
    const auto Gb = 1.0f / Rb;
    const auto Gc = 1.0f / Rc;
    const auto Gd = 1.0f / Rd;
    const auto Ge = 1.0f / Re;
    const auto Gf = 1.0f / Rf;
    R.setSMatrixData ({ { 2 * Ra * (-Ga * Gb * Gc * Gd - Ga * Gb * Gc * Ge - Ga * Gb * Gc * Gf - Ga * Gb * Gd * Ge - Ga * Gb * Ge * Gf - Ga * Gc * Gd * Gf - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) + 1, 2 * Ra * (Ga * Gb * Gc * Gd + Ga * Gb * Gc * Ge + Ga * Gb * Gc * Gf + Ga * Gb * Gd * Ge) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Ra * (Ga * Gb * Gc * Gd + Ga * Gb * Gc * Ge + Ga * Gb * Gc * Gf + Ga * Gc * Gd * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Ra * (Ga * Gb * Gd * Ge - Ga * Gc * Gd * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Ra * (-Ga * Gb * Gd * Ge - Ga * Gb * Ge * Gf - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Ra * (-Ga * Gb * Ge * Gf - Ga * Gc * Gd * Gf - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) },
                        { 2 * Rb * (Ga * Gb * Gc * Gd + Ga * Gb * Gc * Ge + Ga * Gb * Gc * Gf + Ga * Gb * Gd * Ge) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rb * (-Ga * Gb * Gc * Gd - Ga * Gb * Gc * Ge - Ga * Gb * Gc * Gf - Ga * Gb * Gd * Ge - Ga * Gb * Gd * Gf - Gb * Gc * Gd * Gf - Gb * Gc * Ge * Gf - Gb * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) + 1, 2 * Rb * (-Ga * Gb * Gc * Gd - Ga * Gb * Gc * Ge - Ga * Gb * Gc * Gf - Gb * Gc * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rb * (-Ga * Gb * Gd * Ge - Ga * Gb * Gd * Gf - Gb * Gc * Gd * Gf - Gb * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rb * (Ga * Gb * Gd * Ge - Gb * Gc * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rb * (-Ga * Gb * Gd * Gf - Gb * Gc * Gd * Gf - Gb * Gc * Ge * Gf - Gb * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) },
                        { 2 * Rc * (Ga * Gb * Gc * Gd + Ga * Gb * Gc * Ge + Ga * Gb * Gc * Gf + Ga * Gc * Gd * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rc * (-Ga * Gb * Gc * Gd - Ga * Gb * Gc * Ge - Ga * Gb * Gc * Gf - Gb * Gc * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rc * (-Ga * Gb * Gc * Gd - Ga * Gb * Gc * Ge - Ga * Gb * Gc * Gf - Ga * Gc * Gd * Ge - Ga * Gc * Gd * Gf - Gb * Gc * Gd * Ge - Gb * Gc * Ge * Gf - Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) + 1, 2 * Rc * (Ga * Gc * Gd * Ge + Ga * Gc * Gd * Gf + Gb * Gc * Gd * Ge + Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rc * (-Ga * Gc * Gd * Ge - Gb * Gc * Gd * Ge - Gb * Gc * Ge * Gf - Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rc * (Ga * Gc * Gd * Gf - Gb * Gc * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) },
                        { 2 * Rd * (Ga * Gb * Gd * Ge - Ga * Gc * Gd * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rd * (-Ga * Gb * Gd * Ge - Ga * Gb * Gd * Gf - Gb * Gc * Gd * Gf - Gb * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rd * (Ga * Gc * Gd * Ge + Ga * Gc * Gd * Gf + Gb * Gc * Gd * Ge + Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rd * (-Ga * Gb * Gd * Ge - Ga * Gb * Gd * Gf - Ga * Gc * Gd * Ge - Ga * Gc * Gd * Gf - Gb * Gc * Gd * Ge - Gb * Gc * Gd * Gf - Gb * Gd * Ge * Gf - Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) + 1, 2 * Rd * (Ga * Gb * Gd * Ge + Ga * Gc * Gd * Ge + Gb * Gc * Gd * Ge + Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rd * (-Ga * Gb * Gd * Gf - Ga * Gc * Gd * Gf - Gb * Gc * Gd * Gf - Gb * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) },
                        { 2 * Re * (-Ga * Gb * Gd * Ge - Ga * Gb * Ge * Gf - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Re * (Ga * Gb * Gd * Ge - Gb * Gc * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Re * (-Ga * Gc * Gd * Ge - Gb * Gc * Gd * Ge - Gb * Gc * Ge * Gf - Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Re * (Ga * Gb * Gd * Ge + Ga * Gc * Gd * Ge + Gb * Gc * Gd * Ge + Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Re * (-Ga * Gb * Gd * Ge - Ga * Gb * Ge * Gf - Ga * Gc * Gd * Ge - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf - Gb * Gc * Gd * Ge - Gb * Gc * Ge * Gf - Gc * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) + 1, 2 * Re * (-Ga * Gb * Ge * Gf - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf - Gb * Gc * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) },
                        { 2 * Rf * (-Ga * Gb * Ge * Gf - Ga * Gc * Gd * Gf - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rf * (-Ga * Gb * Gd * Gf - Gb * Gc * Gd * Gf - Gb * Gc * Ge * Gf - Gb * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rf * (Ga * Gc * Gd * Gf - Gb * Gc * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rf * (-Ga * Gb * Gd * Gf - Ga * Gc * Gd * Gf - Gb * Gc * Gd * Gf - Gb * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rf * (-Ga * Gb * Ge * Gf - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf - Gb * Gc * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf), 2 * Rf * (-Ga * Gb * Gd * Gf - Ga * Gb * Ge * Gf - Ga * Gc * Gd * Gf - Ga * Gc * Ge * Gf - Ga * Gd * Ge * Gf - Gb * Gc * Gd * Gf - Gb * Gc * Ge * Gf - Gb * Gd * Ge * Gf) / (Ga * Gb * Gd + Ga * Gb * Ge + Ga * Gb * Gf + Ga * Gc * Gd + Ga * Gc * Ge + Ga * Gc * Gf + Ga * Gd * Ge + Ga * Gd * Gf + Gb * Gc * Gd + Gb * Gc * Ge + Gb * Gc * Gf + Gb * Gd * Ge + Gb * Ge * Gf + Gc * Gd * Gf + Gc * Ge * Gf + Gd * Ge * Gf) + 1 } });
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
