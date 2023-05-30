#pragma once

#include <pch.h>

class KingOfToneOverdrive
{
public:
    KingOfToneOverdrive() = default;

    void prepare (float sampleRate)
    {
        R9_C7_Vin.prepare (sampleRate);
        Vbias.setVoltage (4.5f);
    }

    void processBlock (float* x, int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
        {
            R9_C7_Vin.setVoltage (x[n]);

            dp.incident (S2.reflected());
            S2.incident (dp.reflected());

            x[n] = wdft::voltage<float> (RL);
        }
    }

    // Port A:
    wdft::ResistiveCapacitiveVoltageSourceT<float> R9_C7_Vin { 10.0e3f, 0.1e-6f };

    // Port B:
    wdft::ResistiveVoltageSourceT<float> Vbias { 1.0e6f };

    // Port C:
    wdft::ResistorT<float> RL { 1.0e9f };

    // R-type
    struct ImpedanceCalc
    {
        template <typename RType>
        static float calcImpedance (RType& R)
        {
            constexpr float Ag = 100.0f; // op-amp gain
            constexpr float Ri = 1.0e6f; // op-amp input impedance
            constexpr float Ro = 1.0f; // op-amp output impedance

            const auto [Ra, Rb, Rc] = R.getPortImpedances();

            // This scattering matrix was derived using the R-Solver python script (https://github.com/jatinchowdhury18/R-Solver),
            // invoked with command: r_solver.py --adapt 3 --out scratch/king_of_tone_scatt.txt scratch/king_of_tone.txt
            R.setSMatrixData ({ { -((Ag + 1) * Ra * Ra * Rc * Ri + Ra * Ra * Rb * Rc - (Ra * Ra * Rb - Rc * Ri * Ri + (Ra * Ra - Rb * Rb) * Rc + (Ra * Ra - 2 * Rb * Rc) * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Ri * Ri + ((Ag + 1) * Ra * Ra + (Ag + 2) * Ra * Rb) * Rc * Ri + (Ra * Ra * Rb + Ra * Rb * Rb) * Rc - (Ra * Ra * Rb + Ra * Rb * Rb + (Ra + Rc) * Ri * Ri + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc + (Ra * Ra + 2 * Ra * Rb + 2 * (Ra + Rb) * Rc) * Ri) * Ro), (Ag * Ra * Rc * Ri * Ri + Ra * Ra * Rb * Rc + ((2 * Ag + 1) * Ra * Ra + Ag * Ra * Rb) * Rc * Ri - (Ra * Ra * Rb + 2 * (Ra * Ra + Ra * Rb) * Rc + (Ra * Ra + 2 * Ra * Rc) * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Ri * Ri + ((Ag + 1) * Ra * Ra + (Ag + 2) * Ra * Rb) * Rc * Ri + (Ra * Ra * Rb + Ra * Rb * Rb) * Rc - (Ra * Ra * Rb + Ra * Rb * Rb + (Ra + Rc) * Ri * Ri + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc + (Ra * Ra + 2 * Ra * Rb + 2 * (Ra + Rb) * Rc) * Ri) * Ro), -(Ra * Rb + Ra * Ri) * Ro / ((Ag + 1) * Ra * Rc * Ri + Ra * Rb * Rc - (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) * Ro), -(Ra * Rb * Rc + Ra * Rc * Ri - (Ra * Rb + Ra * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Ri + Ra * Rb * Rc - (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) * Ro) },
                                { ((Ag + 1) * Ra * Rb * Rc * Ri + Ra * Rb * Rb * Rc - (Ra * Rb * Rb + 2 * (Ra * Rb + Rb * Rb) * Rc + (Ra * Rb + 2 * Rb * Rc) * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Ri * Ri + ((Ag + 1) * Ra * Ra + (Ag + 2) * Ra * Rb) * Rc * Ri + (Ra * Ra * Rb + Ra * Rb * Rb) * Rc - (Ra * Ra * Rb + Ra * Rb * Rb + (Ra + Rc) * Ri * Ri + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc + (Ra * Ra + 2 * Ra * Rb + 2 * (Ra + Rb) * Rc) * Ri) * Ro), ((Ag + 1) * Ra * Ra * Rc * Ri + (Ag + 1) * Ra * Rc * Ri * Ri - Ra * Rb * Rb * Rc + (Ra * Rb * Rb - (Ra + Rc) * Ri * Ri - (Ra * Ra - Rb * Rb) * Rc - (Ra * Ra + 2 * Ra * Rc) * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Ri * Ri + ((Ag + 1) * Ra * Ra + (Ag + 2) * Ra * Rb) * Rc * Ri + (Ra * Ra * Rb + Ra * Rb * Rb) * Rc - (Ra * Ra * Rb + Ra * Rb * Rb + (Ra + Rc) * Ri * Ri + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc + (Ra * Ra + 2 * Ra * Rb + 2 * (Ra + Rb) * Rc) * Ri) * Ro), -Ra * Rb * Ro / ((Ag + 1) * Ra * Rc * Ri + Ra * Rb * Rc - (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) * Ro), -(Ra * Rb * Rc - Ra * Rb * Ro) / ((Ag + 1) * Ra * Rc * Ri + Ra * Rb * Rc - (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) * Ro) },
                                { -(Ag * Ra * Rb * Rc * Rc * Ri + (Ag * Ag + Ag) * Ra * Rc * Rc * Ri * Ri - ((Ra + 2 * Rb) * Rc * Ri + Rc * Ri * Ri + (Ra * Rb + Rb * Rb) * Rc) * Ro * Ro + ((Ra * Rb + Rb * Rb) * Rc * Rc - (Ag * Ra * Rc + (Ag - 1) * Rc * Rc) * Ri * Ri - (Ag * Ra * Rb * Rc + ((Ag - 1) * Ra + (Ag - 2) * Rb) * Rc * Rc) * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Rc * Ri * Ri + ((Ag + 1) * Ra * Ra + (Ag + 2) * Ra * Rb) * Rc * Rc * Ri + (Ra * Ra * Rb + Ra * Rb * Rb) * Rc * Rc + (Ra * Ra * Rb + Ra * Rb * Rb + (Ra + Rc) * Ri * Ri + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc + (Ra * Ra + 2 * Ra * Rb + 2 * (Ra + Rb) * Rc) * Ri) * Ro * Ro - ((Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((Ag + 2) * Ra * Rc + Rc * Rc) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * (Ra + Rb) * Rc * Rc + ((Ag + 2) * Ra * Ra + (Ag + 4) * Ra * Rb) * Rc) * Ri) * Ro), ((Ag * Ag + 2 * Ag) * Ra * Rc * Rc * Ri * Ri + (Ag * Ra * Ra + 2 * Ag * Ra * Rb) * Rc * Rc * Ri + (Ra * Rc * Ri + (Ra * Ra + Ra * Rb) * Rc) * Ro * Ro - ((Ra * Ra + Ra * Rb) * Rc * Rc + (2 * Ag * Ra * Rc + Ag * Rc * Rc) * Ri * Ri + (((Ag + 1) * Ra + Ag * Rb) * Rc * Rc + (Ag * Ra * Ra + 2 * Ag * Ra * Rb) * Rc) * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Rc * Ri * Ri + ((Ag + 1) * Ra * Ra + (Ag + 2) * Ra * Rb) * Rc * Rc * Ri + (Ra * Ra * Rb + Ra * Rb * Rb) * Rc * Rc + (Ra * Ra * Rb + Ra * Rb * Rb + (Ra + Rc) * Ri * Ri + (Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc + (Ra * Ra + 2 * Ra * Rb + 2 * (Ra + Rb) * Rc) * Ri) * Ro * Ro - ((Ra * Ra + 2 * Ra * Rb + Rb * Rb) * Rc * Rc + ((Ag + 2) * Ra * Rc + Rc * Rc) * Ri * Ri + 2 * (Ra * Ra * Rb + Ra * Rb * Rb) * Rc + (2 * (Ra + Rb) * Rc * Rc + ((Ag + 2) * Ra * Ra + (Ag + 4) * Ra * Rb) * Rc) * Ri) * Ro), -((Ag + 1) * Ra * Rc * Rc * Ri + Ra * Rb * Rc * Rc - (Ra * Rb + Ra * Ri) * Ro * Ro - ((Ra + Rb) * Rc * Rc + Rc * Rc * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Rc * Ri + Ra * Rb * Rc * Rc + (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) * Ro * Ro - (2 * Ra * Rb * Rc + (Ra + Rb) * Rc * Rc + ((Ag + 2) * Ra * Rc + Rc * Rc) * Ri) * Ro), (Ag * Ra * Rc * Ri - ((Ra + Rb) * Rc + Rc * Ri) * Ro) / ((Ag + 1) * Ra * Rc * Ri + Ra * Rb * Rc - (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) * Ro) },
                                { -((Ag + 1) * Rc * Ri + Rb * Rc - (Rb + Ri) * Ro) / ((Ra + Rb) * Rc + Rc * Ri - (Ra + Rb + Ri) * Ro), (Ag * Rc * Ri - Ra * Rc + Ra * Ro) / ((Ra + Rb) * Rc + Rc * Ri - (Ra + Rb + Ri) * Ro), -Ro / (Rc - Ro), 0 } });

            const auto Rd = ((Ag + 1) * Ra * Rc * Ri + Ra * Rb * Rc - (Ra * Rb + (Ra + Rb) * Rc + (Ra + Rc) * Ri) * Ro) / ((Ra + Rb) * Rc + Rc * Ri - (Ra + Rb + Ri) * Ro);
            return Rd;
        }
    };

    using RType = wdft::RtypeAdaptor<float, 3, ImpedanceCalc, decltype (R9_C7_Vin), decltype (Vbias), decltype (RL)>;
    RType R { R9_C7_Vin, Vbias, RL };

    // Port D:
    wdft::ResistorT<float> R10 { 220.0e3f };
    wdft::WDFParallelT<float, decltype (R), decltype (R10)> P1 { R, R10 };
    wdft::ResistorT<float> R11 { 6.8e3f };
    wdft::WDFSeriesT<float, decltype (R11), decltype (P1)> S2 { R11, P1 };
    wdft::DiodePairT<float, decltype (S2)> dp { S2, 2.9849127806230505e-10f, 25.85e-3f, 3.187726462543485f };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KingOfToneOverdrive)
};
