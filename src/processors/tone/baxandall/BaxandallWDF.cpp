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
}
