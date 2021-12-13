#pragma once

#include <pch.h>

/**
 * Implentation based on Werner et. al:
 * https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8371321
 */
class BaxandallWDF
{
public:
    BaxandallWDF() = default;

    void prepare (double fs);
    void setParams (float bassParam, float trebleParam);

    inline float processSample (float x)
    {
        Vin.setVoltage (x);
        R.incident (0.0f);

        return wdft::voltage<float> (Rl);
    }

private:
    static constexpr auto Pt = 100.0e3f;
    static constexpr auto Pb = 100.0e3f;

    // Port A
    wdft::ResistorT<float> Pt_plus { Pt * 0.5f };
    wdft::ResistorT<float> Resd { 10.0e3f };
    wdft::WDFParallelT<float, decltype (Pt_plus), decltype (Resd)> P4 { Pt_plus, Resd };
    wdft::CapacitorT<float> Cd { 6.4e-9f };
    wdft::WDFSeriesT<float, decltype (Cd), decltype (P4)> S4 { Cd, P4 };

    // Port B
    wdft::ResistorT<float> Pt_minus { Pt * 0.5f };
    wdft::ResistorT<float> Rese { 1.0e3f };
    wdft::WDFParallelT<float, decltype (Pt_minus), decltype (Rese)> P5 { Pt_minus, Rese };
    wdft::CapacitorT<float> Ce { 64.0e-9f };
    wdft::WDFSeriesT<float, decltype (Ce), decltype (P5)> S5 { Ce, P5 };
    wdft::ResistorT<float> Rl { 1.0e6f };
    wdft::WDFParallelT<float, decltype (Rl), decltype (S5)> P1 { Rl, S5 };

    // Port C
    wdft::ResistorT<float> Resc { 10.0e3f };

    // Port D
    wdft::ResistorT<float> Pb_minus { Pb * 0.5f };
    wdft::CapacitorT<float> Cc { 220.0e-9f };
    wdft::WDFParallelT<float, decltype (Pb_minus), decltype (Cc)> P3 { Pb_minus, Cc };
    wdft::ResistorT<float> Resb { 1.0e3f };
    wdft::WDFSeriesT<float, decltype (Resb), decltype (P3)> S3 { Resb, P3 };

    // Port E
    wdft::ResistorT<float> Pb_plus { Pb * 0.5f };
    wdft::CapacitorT<float> Cb { 22.0e-9f };
    wdft::WDFParallelT<float, decltype (Pb_plus), decltype (Cb)> P2 { Pb_plus, Cb };
    wdft::ResistorT<float> Resa { 10.0e3f };
    wdft::WDFSeriesT<float, decltype (Resa), decltype (P2)> S2 { Resa, P2 };

    // Port F
    wdft::ResistiveVoltageSourceT<float> Vin; // @TODO make this ideal when we make the R-node adaptable
    wdft::CapacitorT<float> Ca { 1.0e-6f };
    wdft::WDFSeriesT<float, decltype (Vin), decltype (Ca)> S1 { Vin, Ca };

    using RType = wdft::RootRtypeAdaptor<float, decltype (S4), decltype (P1), decltype (Resc), decltype (S3), decltype (S2), decltype (S1)>;
    RType R { std::tie (S4, P1, Resc, S3, S2, S1) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaxandallWDF)
};
