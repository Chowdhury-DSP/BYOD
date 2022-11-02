#pragma once

#include "ModifiedRType.h"
#include <pch.h>

template <typename Float, typename TriodeModelType>
struct JuniorBWDF
{
    explicit JuniorBWDF (TriodeModelType& model) : triode_model (model) {}

    void prepare (Float sampleRate)
    {
        R.prepare (sampleRate);

        C24.prepare (sampleRate);
        C22.prepare (sampleRate);
        C2.prepare (sampleRate);
        C1.prepare (sampleRate);

        Vbb.setVoltage ((Float) 232.0);
    }

    inline Float process (Float x) noexcept
    {
        Vsig.setVoltage (x);
        R.incident (triode_model.compute (R.reflected()));
        return wdft::voltage<Float> (R6);
    }

    wdft::ResistiveVoltageSourceT<Float> Vsig { (Float) 1.0e3 };
    wdft::PolarityInverterT<Float, decltype (Vsig)> Isig { Vsig };

    wdft::CapacitorT<Float> C24 { (Float) 220.0e-12 };
    wdft::WDFParallelT<Float, decltype (Isig), decltype (C24)> P0 { Isig, C24 };

    wdft::ResistorT<Float> R01 { (Float) 10.0e3 };
    wdft::WDFSeriesT<Float, decltype (P0), decltype (R01)> S0 { P0, R01 };

    wdft::ResistorT<Float> R2 { (Float) 1.0e6 };
    wdft::WDFParallelT<Float, decltype (S0), decltype (R2)> P1 { S0, R2 };

    // cathode
    wdft::ResistorT<Float> R4 { (Float) 1.5e3 };
    wdft::CapacitorT<Float> C22 { (Float) 47.0e-6 };
    wdft::WDFParallelT<Float, decltype (R4), decltype (C22)> P2 { R4, C22 };

    // plate
    wdft::CapacitorT<Float> C2 { (Float) 22.0e-12 };
    wdft::ResistorT<Float> R5 { (Float) 470.0e3 };
    wdft::WDFParallelT<Float, decltype (C2), decltype (R5)> P4 { C2, R5 };

    wdft::ResistorT<Float> R6 { (Float) 22.0e3 };
    wdft::WDFSeriesT<Float, decltype (P4), decltype (R6)> S2 { P4, R6 };

    wdft::CapacitorT<Float> C1 { (Float) 0.022e-6 };
    wdft::WDFSeriesT<Float, decltype (C1), decltype (S2)> S1 { C1, S2 };

    // voltage source includes Rb as series resistor
    wdft::ResistiveVoltageSourceT<Float> Vbb { (Float) 125.0e3 };
    wdft::WDFParallelT<Float, decltype (Vbb), decltype (S1)> P3 { Vbb, S1 };

    struct ImpedanceCalc
    {
        template <typename RType>
        static void calcImpedance (RType& R)
        {
            const auto fs = R.getSampleRate();
            const auto sampleRateFactor = fs / (Float) 96000;
            const auto Rcomp = (Float) 1000 / std::pow (sampleRateFactor, sampleRateFactor <= (Float) 1 ? (Float) 0.2 : (Float) 0.29);

            // always use 1k for the upward facing ports
            const auto Rg = Rcomp;
            const auto Rp = Rcomp;

            // set port resistances
            const auto [Ri, Rk, Ro] = R.getPortImpedances();

            // This scattering matrix was derived using the R-Solver python script (https://github.com/jatinchowdhury18/R-Solver),
            // invoked with command: r_solver.py --out scratch/cathode-scatt.txt netlists/cathodbiasnets-final.txt
            R.setSMatrixData ({ { -((Rg - Ri) * Rk + (Rg - Ri - Rk) * Ro + (Rg - Ri - Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * Rg * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg * Rk + Rg * Ro + Rg * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * (Rg * Ro + Rg * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * Rg * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) },
                                { 2 * Rk * Rp / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro - (Rg + Ri + Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * Rk * Rp / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * (Rg + Ri) * Rp / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg + Ri + Rk) * Rp / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) },
                                { 2 * (Ri * Rk + Ri * Ro + Ri * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * Ri * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), ((Rg - Ri) * Rk + (Rg - Ri + Rk) * Ro + (Rg - Ri + Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Ri * Ro + Ri * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * Ri * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) },
                                { -2 * (Rk * Ro + Rk * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * (Rg + Ri) * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rk * Ro + Rk * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -((Rg + Ri) * Rk - (Rg + Ri - Rk) * Ro - (Rg + Ri - Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg + Ri) * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) },
                                { -2 * Rk * Ro / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg + Ri + Rk) * Ro / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * Rk * Ro / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg + Ri) * Ro / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), ((Rg + Ri) * Rk - (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) } });
        }
    };

    using RType = wdft::RtypeAdaptorMultN<Float, 2, ImpedanceCalc, decltype (P1), decltype (P2), decltype (P3)>;
    RType R { P1, P2, P3 };

    TriodeModelType& triode_model;
};
