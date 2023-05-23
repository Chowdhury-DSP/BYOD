#include "BaxandallWDF.h"

void BaxandallWDF::prepare (double fs)
{
    Ca.prepare ((float) fs);
    Pb_plus_Cb.prepare ((float) fs);
    Pb_minus_Cc.prepare ((float) fs);
    Pt_plus_Resd_Cd.prepare ((float) fs);
    Pt_minus_Rese_Ce.prepare ((float) fs);
}

void BaxandallWDF::setParams (float bassParam, float trebleParam)
{
    {
        chowdsp::wdft::ScopedDeferImpedancePropagation deferImpedance { P1, S2, S3, Pt_plus_Resd_Cd };

        Pb_plus_Cb.setResistanceValue (Pb * bassParam);
        Pb_minus_Cc.setResistanceValue (Pb * (1.0f - bassParam));

        Pt_plus_Resd_Cd.setResistanceValue (parallel_resistors (Pt * trebleParam, Resd));
        Pt_minus_Rese_Ce.setResistanceValue (parallel_resistors (Pt * (1.0f - trebleParam), Rese));
    }

    R.propagateImpedanceChange();
}
