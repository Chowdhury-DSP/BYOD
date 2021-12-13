#include "BaxandallWDF.h"

void BaxandallWDF::prepare (double fs)
{
    Ca.prepare ((float) fs);
    Cb.prepare ((float) fs);
    Cc.prepare ((float) fs);
    Cd.prepare ((float) fs);
    Ce.prepare ((float) fs);
}

void BaxandallWDF::setParams (float bassParam, float trebleParam)
{
    Pb_plus.setResistanceValue (Pb * bassParam);
    Pb_minus.setResistanceValue (Pb * (1.0f - bassParam));

    Pt_plus.setResistanceValue (Pt * trebleParam);
    Pt_minus.setResistanceValue (Pt * (1.0f - trebleParam));
    
    const auto Ra = S4.R;
    const auto Rb = P1.R;
    const auto Rc = Resc.R;
    const auto Rd = S3.R;
    const auto Re = S2.R;
    const auto Rf = S1.R;
    const auto Ga = 1.0f / Ra;
    const auto Gb = 1.0f / Rb;
    const auto Gc = 1.0f / Rc;
    const auto Gd = 1.0f / Rd;
    const auto Ge = 1.0f / Re;
    const auto Gf = 1.0f / Rf;
    // R.setSMatrixData...
}
