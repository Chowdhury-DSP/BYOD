#ifndef PREAMPSTAGE_H_INCLUDED
#define PREAMPSTAGE_H_INCLUDED

#include <pch.h>

namespace GainStageSpace
{
using namespace chowdsp::WDFT;

class PreAmpWDF
{
public:
    PreAmpWDF (double sampleRate);

    void setGain (float gain);
    void reset();

    inline float getFF1() noexcept
    {
        return (float) current<double> (Vbias2);
    }

    inline float processSample (float x)
    {
        Vin.setVoltage ((double) x);

        Vin.incident (I1.reflected());
        auto y = voltage<double> (Vbias) + voltage<double> (R6);
        I1.incident (Vin.reflected());

        return (float) y;
    }

private:
    using Capacitor = CapacitorT<double>;
    using Resistor = ResistorT<double>;
    using ResVs = ResistiveVoltageSourceT<double>;

    Capacitor C3;
    Capacitor C5;
    Capacitor C16;

    Resistor R6 { 10000.0 };
    Resistor R7 { 1500.0 };

    ResVs Vbias2 { 15000.0 };
    ResVs Vbias;

    WDFParallelT<double, Capacitor, Resistor> P1 { C5, R6 };
    WDFSeriesT<double, decltype (P1), ResVs> S1 { P1, Vbias };

    WDFParallelT<double, ResVs, Capacitor> P2 { Vbias2, C16 };
    WDFSeriesT<double, decltype (P2), Resistor> S2 { P2, R7 };
    WDFParallelT<double, decltype (S1), decltype (S2)> P3 { S1, S2 };

    WDFSeriesT<double, decltype (P3), Capacitor> S3 { P3, C3 };
    PolarityInverterT<double, decltype (S3)> I1 { S3 };

    IdealVoltageSourceT<double, decltype (I1)> Vin { I1 };
};
} // namespace GainStageSpace

#endif // PREAMPSTAGE_H_INCLUDED
