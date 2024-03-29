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
        Vin_C1.prepare ((float) fs);
        Vin_C1.incident (-4.83726311f);
        Vin_C1.reflected();

        C2.prepare ((float) fs);
        C2.incident (4.83662891f);
        C2.reflected();

        C6.prepare ((float) fs);
        C6.incident (-5.02740574f);
        C6.reflected();

        C7.prepare ((float) fs);
        C7.incident (5.07801962f);
        C7.reflected();

        C8_C9.prepare ((float) fs);
        C8_C9.incident (5.0518117f);
        C8_C9.reflected();

        Vb.setVoltage (4.9f);
    }

    void reset()
    {
        Vin_C1.reset();
        C2.reset();
        C6.reset();
        C7.reset();
        C8_C9.reset();
    }

    void setParams (float driveParam, float presenceParam, float lowCutHz)
    {
        chowdsp::wdft::ScopedDeferImpedancePropagation deferImpedance { R };

        R4_Rdrive.setResistanceValue (R4 + driveParam * Rdrive);

        Rp_m.setResistanceValue (presenceParam * Rpresence);
        Rp_p.setResistanceValue ((1.0f - presenceParam) * Rpresence);

        const auto c8c9Val = 1.0f / (juce::MathConstants<float>::twoPi * RlevelVal * lowCutHz);
        C8_C9.setCapacitanceValue (c8c9Val);
    }

    template <FlapjackClipMode clipMode>
    inline float processSample (float x)
    {
        Vin_C1.setVoltage (x);
        R.compute<clipMode>();

        return wdft::voltage<float> (Rlevel);
    }

    // Port A
    wdft::CapacitiveVoltageSourceT<float> Vin_C1 { 10.0e-9f };
    wdft::ResistiveVoltageSourceT<float> Vb { 1.0e6f };
    wdft::WDFParallelT<float, decltype (Vin_C1), decltype (Vb)> Pin { Vin_C1, Vb };
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
    static constexpr float RlevelVal = 47.0e3f;
    wdft::ResistorT<float> Rlevel { RlevelVal };
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
