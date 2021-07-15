#pragma once

#include <pch.h>

class TubeProc
{
public:
    TubeProc (double sampleRate) : Cpk (0.33e-12, sampleRate, alpha),
                                   Cgp (1.7e-12, sampleRate, alpha),
                                   Cgk (1.6e-12, sampleRate, alpha)
    {
        Vp = 0.0;
    }

    template <typename T>
    inline int sgn (T val) const noexcept
    {
        return (T (0) < val) - (val < T (0));
    }

    template <typename T>
    inline T pwrs (T x, T y) const noexcept
    {
        return sgn (x) * std::pow (std::abs (x), y);
    }

    inline double getCurrent (double _Vg, double _Vp) const noexcept
    {
        return 2.0e-9 * pwrs (0.1 * _Vg - 0.01 * _Vp, 0.33);
    }

    inline double processSample (double Vg) noexcept
    {
        Vin.setVoltage (Vg);
        Is.setCurrent (getCurrent (Vg, Vp));

        D1.incident (S2.reflected());
        S2.incident (D1.reflected());
        Vp = wdft::voltage<double> (Cpk);

        return Vp;
    }

    inline void processBlock (double* buffer, const int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
            buffer[n] = processSample (buffer[n]);
    }

private:
    using ResIs = wdft::ResistiveCurrentSourceT<double>;
    using ResVs = wdft::ResistiveVoltageSourceT<double>;
    using Res = wdft::ResistorT<double>;
    using Cap = wdft::CapacitorAlphaT<double>;

    ResIs Is;
    ResVs Vin;
    Res Rgk { 2.7e3 };

    Cap Cpk;
    Cap Cgp;
    Cap Cgk;

    wdft::WDFParallelT<double, Cap, ResIs> P1 { Cpk, Is };
    wdft::WDFSeriesT<double, Cap, decltype (P1)> S1 { Cgp, P1 };
    wdft::WDFParallelT<double, Cap, decltype (S1)> P2 { Cgk, S1 };
    wdft::PolarityInverterT<double, ResVs> I1 { Vin };
    wdft::WDFParallelT<double, decltype (I1), decltype (P2)> P3 { I1, P2 };
    wdft::WDFSeriesT<double, Res, decltype (P3)> S2 { Rgk, P3 };

    wdft::DiodeT<double, decltype (S2)> D1 { S2, 1.0e-10 };

    double Vp = 0.0;
    static constexpr double alpha = 0.4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeProc)
};
