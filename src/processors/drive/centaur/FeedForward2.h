#ifndef FEEDFORWARD2_H_INCLUDED
#define FEEDFORWARD2_H_INCLUDED

#include <pch.h>

namespace GainStageSpace
{
using namespace chowdsp::wdft;

class FeedForward2WDF
{
public:
    explicit FeedForward2WDF (double sampleRate);

    void reset();
    void setGain (float gain);

    inline float processSample (float x)
    {
        Vin.setVoltage ((double) x);

        Vin.incident (I1.reflected());
        I1.incident (Vin.reflected());
        auto y = current<double> (R16);

        return (float) y;
    }

private:
    using Capacitor = CapacitorT<double>;
    using Resistor = ResistorT<double>;
    using ResVs = ResistiveVoltageSourceT<double>;

    Resistor R5 { 5100.0 };
    Resistor R8 { 1500.0 };
    Resistor R9 { 1000.0 };
    Resistor RVTop { 50000.0 };
    Resistor RVBot { 50000.0 };
    Resistor R15 { 22000.0 };
    Resistor R16 { 47000.0 };
    Resistor R17 { 27000.0 };
    Resistor R18 { 12000.0 };
    ResVs Vbias;

    Capacitor C4;
    Capacitor C6;
    Capacitor C11;
    Capacitor C12;

    WDFSeriesT<double, Capacitor, Resistor> S1 { C12, R18 };
    WDFParallelT<double, decltype (S1), Resistor> P1 { S1, R17 };

    WDFSeriesT<double, Capacitor, Resistor> S2 { C11, R15 };
    WDFSeriesT<double, decltype (S2), Resistor> S3 { S2, R16 };
    WDFParallelT<double, decltype (S3), decltype (P1)> P2 { S3, P1 };
    WDFParallelT<double, decltype (P2), Resistor> P3 { P2, RVBot };
    WDFSeriesT<double, decltype (P3), Resistor> S4 { P3, RVTop };

    WDFSeriesT<double, Capacitor, Resistor> S5 { C6, R9 };
    WDFParallelT<double, decltype (S4), decltype (S5)> P4 { S4, S5 };
    WDFParallelT<double, decltype (P4), Resistor> P5 { P4, R8 };
    WDFSeriesT<double, decltype (P5), ResVs> S6 { P5, Vbias };

    WDFParallelT<double, Resistor, Capacitor> P6 { R5, C4 };
    WDFSeriesT<double, decltype (P6), decltype (S6)> S7 { P6, S6 };
    PolarityInverterT<double, decltype (S7)> I1 { S7 };

    IdealVoltageSourceT<double, decltype (I1)> Vin { I1 };
};
} // namespace GainStageSpace

#endif // FEEDFORWARD2_H_INCLUDED
