#pragma once

#include "../diode_circuits/OmegaProvider.h"
#include <pch.h>

// This circuit model was originally implemented as part of Sam Schachter's
// Master's Thesis (https://github.com/schachtersam32/WaveDigitalFilters_Sharc/blob/master/MXR_DistPlus.h).
// Since then, we've re-derived the R-adaptor to adapt to the port facing the diode pair.
class MXRDistWDF
{
public:
    MXRDistWDF() = default;

    void prepare (double sampleRate)
    {
        C1.prepare ((float) sampleRate);
        R1_C2.prepare ((float) sampleRate);
        ResDist_R3_C3.prepare ((float) sampleRate);
        R5_C4.prepare ((float) sampleRate);
        C5.prepare ((float) sampleRate);

        Vb.setVoltage (4.5f);
    }

    void setParams (float distParam)
    {
        ResDist_R3_C3.setResistanceValue (distParam * rDistVal + R3Val);
    }

    inline float processSample (float x)
    {
        Vin.setVoltage (x);

        DP.incident (P3.reflected());
        P3.incident (DP.reflected());

        return wdft::voltage<float> (Rout);
    }

    // Port A
    wdft::ResistorT<float> R4 { 1.0e6f };

    // Port B
    wdft::ResistiveVoltageSourceT<float> Vin;
    wdft::CapacitorT<float> C1 { 1.0e-9f };
    wdft::WDFParallelT<float, decltype (Vin), decltype (C1)> P1 { Vin, C1 };

    wdft::ResistorCapacitorSeriesT<float> R1_C2 { 10.0e3f, 10.0e-9f };

    wdft::WDFSeriesT<float, decltype (R1_C2), decltype (P1)> S2 { R1_C2, P1 };
    wdft::ResistiveVoltageSourceT<float> Vb { 1.0e6f }; // encompasses R2
    wdft::WDFParallelT<float, decltype (Vb), decltype (S2)> P2 { Vb, S2 };

    // Port C
    static constexpr float R3Val = 4.7e3f;
    static constexpr float rDistVal = 1.0e6f;
    wdft::ResistorCapacitorSeriesT<float> ResDist_R3_C3 { rDistVal + R3Val, 47.0e-9f };

    struct ImpedanceCalc
    {
        template <typename RType>
        static float calcImpedance (RType& R)
        {
            constexpr float A = 100.0f; // op-amp gain
            constexpr float Ri = 1.0e9f; // op-amp input impedance
            constexpr float Ro = 1.0e-1f; // op-amp output impedance

            const auto [Ra, Rb, Rc] = R.getPortImpedances();

            // This scattering matrix was derived using the R-Solver python script (https://github.com/jatinchowdhury18/R-Solver),
            // invoked with command: r_solver.py --datum 0 --adapt 3 --out scratch/mxr_scatt.txt netlists/mxr_distplus_vcvs.txt
            R.setSMatrixData ({ { -(Ra * Ra * Rb * Rb + 2 * Ra * Ra * Rb * Rc + (Ra * Ra - Rb * Rb) * Rc * Rc - ((A + 1) * Rc * Rc - Ra * Ra) * Ri * Ri - ((A + 2) * Rb * Rc * Rc - 2 * Ra * Ra * Rb - 2 * Ra * Ra * Rc) * Ri + (Rb * Rb * Rc + Rb * Rc * Rc + Rc * Ri * Ri + (2 * Rb * Rc + Rc * Rc) * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), -(2 * Ra * Ra * Rb * Rc + 2 * (Ra * Ra + Ra * Rb) * Rc * Rc - (A * Ra * Ra + A * Ra * Rc) * Ri * Ri - (A * Ra * Ra * Rb - (A + 2) * Ra * Rc * Rc + ((A - 2) * Ra * Ra + A * Ra * Rb) * Rc) * Ri - (Ra * Rb * Rc + Ra * Rc * Rc + Ra * Rc * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), -(2 * Ra * Ra * Rb * Rb + ((A + 2) * Ra * Ra + 2 * (A + 1) * Ra * Rc) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + ((A + 4) * Ra * Ra * Rb + ((A + 2) * Ra * Ra + 2 * (A + 2) * Ra * Rb) * Rc) * Ri - (Ra * Rb * Rb + Ra * Rb * Rc + Ra * Ri * Ri + (2 * Ra * Rb + Ra * Rc) * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), (Ra * Rb + Ra * Rc + Ra * Ri) / (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) },
                                { -(2 * Ra * Rb * Rb * Rc + 2 * (Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Rb * Rc * Rc + 2 * Ra * Rb * Rc) * Ri - (Rb * Rb * Rc + Rb * Rc * Rc + Rb * Rc * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), -(Ra * Ra * Rb * Rb + 2 * Ra * Rb * Rb * Rc - (Ra * Ra - Rb * Rb) * Rc * Rc - ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri - ((A + 2) * Ra * Rc * Rc + 2 * Ra * Ra * Rc) * Ri - (Ra * Rb * Rb + Rb * Rb * Rc - Ra * Rc * Rc - (Ra + Rc) * Ri * Ri - (2 * Ra * Rc + Rc * Rc) * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), (2 * Ra * Ra * Rb * Rb + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + ((A + 2) * Ra * Rb * Rc + 2 * Ra * Ra * Rb) * Ri - (2 * Ra * Rb * Rb + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + Rb * Rc) * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), Rb * Rc / (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) },
                                { -(2 * Ra * Rb * Rb * Rc + 2 * (Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Rc * Rc + 2 * Ra * Rc) * Ri * Ri + (4 * Ra * Rb * Rc + ((A + 4) * Rb + 2 * Ra) * Rc * Rc) * Ri - (Rb * Rb * Rc + Rb * Rc * Rc + Rc * Ri * Ri + (2 * Rb * Rc + Rc * Rc) * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), (2 * Ra * Ra * Rb * Rc + 2 * (Ra * Ra + Ra * Rb) * Rc * Rc + (A * Ra * Rc + A * Rc * Rc) * Ri * Ri + ((2 * (A + 1) * Ra + A * Rb) * Rc * Rc + (A * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (2 * Ra * Rb * Rc + (2 * Ra + Rb) * Rc * Rc + (2 * Ra * Rc + Rc * Rc) * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), (Ra * Ra * Rb * Rb - (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc - ((A + 1) * Rc * Rc - Ra * Ra) * Ri * Ri + (2 * Ra * Ra * Rb - ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc) * Ri - (Ra * Rb * Rb - (Ra + Rb) * Rc * Rc + Ra * Ri * Ri + (2 * Ra * Rb - Rc * Rc) * Ri) * Ro) / (Ra * Ra * Rb * Rb + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((A + 2) * Ra * Rc + (A + 1) * Rc * Rc + Ra * Ra) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * Ra * Ra * Rb + ((A + 2) * Ra + (A + 2) * Rb) * Rc * Rc + ((A + 4) * Ra * Rb + 2 * Ra * Ra) * Rc) * Ri - (Ra * Rb * Rb + (Ra + Rb) * Rc * Rc + (Ra + Rc) * Ri * Ri + (2 * Ra * Rb + Rb * Rb) * Rc + (2 * Ra * Rb + 2 * (Ra + Rb) * Rc + Rc * Rc) * Ri) * Ro), (Rb * Rc + Rc * Ri) / (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) },
                                { (A * Rc * Ri - (Rb + Rc + Ri) * Ro) / (Ra * Rb + (Ra + Rb) * Rc + ((A + 1) * Rc + Ra) * Ri - (Rb + Rc + Ri) * Ro), ((A * Ra + A * Rc) * Ri - Rc * Ro) / (Ra * Rb + (Ra + Rb) * Rc + ((A + 1) * Rc + Ra) * Ri - (Rb + Rc + Ri) * Ro), -(A * Ra * Ri + (Rb + Ri) * Ro) / (Ra * Rb + (Ra + Rb) * Rc + ((A + 1) * Rc + Ra) * Ri - (Rb + Rc + Ri) * Ro), 0 } });

            const auto Rd = -(Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) * Ro / (Ra * Rb + (Ra + Rb) * Rc + ((A + 1) * Rc + Ra) * Ri - (Rb + Rc + Ri) * Ro);
            return Rd;
        }
    };

    wdft::RtypeAdaptor<float, 3, ImpedanceCalc, decltype (R4), decltype (P2), decltype (ResDist_R3_C3)> R { R4, P2, ResDist_R3_C3 };

    // Port D
    wdft::ResistorCapacitorSeriesT<float> R5_C4 { 10.0e3f, 1.0e-6f };
    wdft::WDFSeriesT<float, decltype (R5_C4), decltype (R)> S7 { R5_C4, R };

    wdft::ResistorT<float> Rout { 10.0e3f };
    wdft::WDFParallelT<float, decltype (Rout), decltype (S7)> P4 { Rout, S7 };
    wdft::CapacitorT<float> C5 { 1.0e-9f };
    wdft::WDFParallelT<float, decltype (C5), decltype (P4)> P3 { C5, P4 };

    wdft::DiodePairT<float, decltype (P3), wdft::DiodeQuality::Best, OmegaProvider> DP { P3, 2.52e-9f, 25.85e-3f * 1.75f };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MXRDistWDF)
};
