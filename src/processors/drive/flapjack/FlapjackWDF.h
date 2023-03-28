#pragma once

#include "FlapjackOpAmpRType.h"
#include "FlapjackWDFScatteringMatrix.h"
#include <pch.h>

class FlapjackWDF
{
public:
    FlapjackWDF() = default;

    void prepare (double fs)
    {
        C1.prepare ((float) fs);
        C2.prepare ((float) fs);
        C6.prepare ((float) fs);
        C7.prepare ((float) fs);
        C8_C9.prepare ((float) fs);
        Vb.setVoltage (4.9f);
    }

    void reset()
    {
        C1.reset();
        C2.reset();
        C6.reset();
        C7.reset();
        C8_C9.reset();
    }

    void setParams (float driveParam, float presenceParam)
    {
        R4_Rdrive.setResistanceValue (R4 + driveParam * Rdrive); // maybe add some skew on this parameter?

        Rp_p.setResistanceValue (presenceParam * Rpresence);
        Rp_p.setResistanceValue ((1.0f - presenceParam) * Rpresence);
    }

    inline float processSample (float x)
    {
        Vin.setVoltage (x);
        R.compute();

        return wdft::voltage<float> (Rlevel);
    }

private:
    // Port A
    wdft::ResistiveVoltageSourceT<float> Vin;
    wdft::CapacitorT<float> C1 { 10.0e-9f };
    wdft::WDFSeriesT<float, decltype (Vin), decltype (C1)> Sin { Vin, C1 };
    wdft::ResistiveVoltageSourceT<float> Vb { 1.0e6f };
    wdft::WDFParallelT<float, decltype (Sin), decltype (Vb)> Pin { Sin, Vb };
    wdft::ResistorT<float> R2 { 10.0e3f };
    wdft::WDFSeriesT<float, decltype (R2), decltype (Pin)> Sa { R2, Pin };

    // Port B
    wdft::CapacitorT<float> C2 { 10.0e-6f };
    static constexpr auto R4 = 220.0f;
    static constexpr auto Rdrive = 47.0e3f;
    wdft::ResistorT<float> R4_Rdrive { R4 };
    wdft::WDFSeriesT<float, decltype (C2), decltype (R4_Rdrive)> Sb { C2, R4_Rdrive };

    // Port C
    wdft::ResistorT<float> R3 { 100.0e3f };

    // Port D
    wdft::ResistorT<float> R5 { 10.0e3f };

    // Port E
    wdft::ResistorT<float> R6 { 10.0e3f };

    // Port F
    wdft::CapacitorT<float> C6 { 22.0e-9f };

    // Port G
    static constexpr auto Rpresence = 47.0e3f;
    wdft::ResistorT<float> Rp_p { 0.5f * Rpresence };

    // Port H
    wdft::ResistorT<float> Rp_m { 0.5f * Rpresence };

    // Port J
    wdft::CapacitorT<float> C7 { 82.0e-9f };
    wdft::ResistorT<float> R7 { 1.0e3f };
    wdft::WDFSeriesT<float, decltype (C7), decltype (R7)> Sj { C7, R7 };

    // Port K
    wdft::CapacitorT<float> C8_C9 { 47.0e-9f };
    wdft::ResistorT<float> Rlevel { 47.0e3f };
    wdft::WDFSeriesT<float, decltype (C8_C9), decltype (Rlevel)> Sk { C8_C9, Rlevel };

    struct ImpedanceCalc
    {
        template <typename RType>
        static void calcImpedance (RType& R)
        {
            const auto [Ra, Rb, Rc, Rd, Re, Rf, Rg, Rh, Rj, Rk] = R.getPortImpedances();
            static constexpr float Ag = 100000.0f; // op-amp gain
            static constexpr float Ri = 500.0e6f; // op-amp input impedance
            static constexpr float Ro = 1.0e-1f; // op-amp output impedance
            matrix_setter (Ra, Rb, Rc, Rd, Re, Rf, Rg, Rh, Rj, Rk, Ri, Ro, Ag, R);
        }
    };

    using RType = FlapjackOpAmpRType<float,
                                         ImpedanceCalc,
                                         decltype (Sa),
                                         decltype (Sb),
                                         decltype (R3),
                                         decltype (R5),
                                         decltype (R6),
                                         decltype (C6),
                                         decltype (Rp_p),
                                         decltype (Rp_m),
                                         decltype (Sj),
                                         decltype (Sk)>;
    RType R { Sa, Sb, R3, R5, R6, C6, Rp_p, Rp_m, Sj, Sk };
};
