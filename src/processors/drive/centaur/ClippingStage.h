#ifndef CLIPPINGSTAGE_H_INCLUDED
#define CLIPPINGSTAGE_H_INCLUDED

// #include "DiodePair.h"
#include <pch.h>

namespace GainStageSpace
{
using namespace chowdsp::WDFT;

class ClippingWDF
{
public:
    ClippingWDF (double sampleRate) : C9 (1.0e-6, sampleRate),
                                      C10 (1.0e-6, sampleRate)
    { reset(); }

    void reset() { Vbias.setVoltage (0.0f); }

    inline float processSample (float x)
    {
        Vin.setVoltage ((double) x);

        D23.incident (P1.reflected());
        P1.incident (D23.reflected());
        auto y = current<double> (C10);

        return (float) y;
    }

private:
    using Capacitor = CapacitorT<double>;
    using Resistor = ResistorT<double>;
    using ResVs = ResistiveVoltageSourceT<double>;

    ResVs Vin;
    Capacitor C9;
    Resistor R13 { 1000.0 };

    Capacitor C10;
    ResVs Vbias { 47000.0 };

    PolarityInverterT<double, ResVs> I1 { Vin };
    WDFSeriesT<double, decltype (I1), Capacitor> S1 { I1, C9 };
    WDFSeriesT<double, decltype (S1), Resistor> S2 { S1, R13 };

    WDFSeriesT<double, Capacitor, ResVs> S3 { C10, Vbias };
    WDFParallelT<double, decltype (S2), decltype (S3)> P1 { S2, S3 };

    DiodePairT<double, decltype (P1)> D23 { P1, 15e-6, 0.02585 };
};

} // namespace GainStageSpace

#endif // CLIPPINGSTAGE_H_INCLUDED
