#pragma once

#include <pch.h>

class MouseDriveWDF
{
public:
    MouseDriveWDF() = default;

    void prepare (double sampleRate)
    {
        Vin_C1.prepare ((float) sampleRate);
        C2.prepare ((float) sampleRate);
        Rd_C4.prepare ((float) sampleRate);
        R4_C5.prepare ((float) sampleRate);
        R5_C6.prepare ((float) sampleRate);
        R6_C7.prepare ((float) sampleRate);

        R2.setVoltage (4.5f);
    }

    inline float process (float x) noexcept
    {
        Vin_C1.setVoltage (x);
        diodes.incident (Sd.reflected());
        const auto y = wdft::voltage<float> (diodes);
        Sd.incident (diodes.reflected());
        return y;
    }

    // Port A
    wdft::CapacitiveVoltageSourceT<float> Vin_C1 { 22.0e-9f };
    wdft::ResistiveVoltageSourceT<float> R2 { 1.0e6f };
    wdft::WDFParallelT<float, decltype (Vin_C1), decltype (R2)> P1 { Vin_C1, R2 };

    wdft::ResistorT<float> R3 { 1.0e3f };
    wdft::WDFSeriesT<float, decltype (P1), decltype (R3)> S2 { P1, R3 };

    wdft::CapacitorT<float> C2 { 1.0e-9f };
    wdft::WDFParallelT<float, decltype (S2), decltype (C2)> Pa { S2, C2 };

    // Port B
    wdft::ResistorCapacitorSeriesT<float> R4_C5 { 47.0f, 2.2e-6f };
    wdft::ResistorCapacitorSeriesT<float> R5_C6 { 560.0f, 4.7e-6f };
    wdft::WDFParallelT<float, decltype (R4_C5), decltype (R5_C6)> Pb { R4_C5, R5_C6 };

    // Port C
    static constexpr float Rdistortion = 100.0e3f;
    wdft::ResistorCapacitorParallelT<float> Rd_C4 { 0.5f * Rdistortion, 100.0e-12f };

    // R-Type
    struct ImpedanceCalc
    {
        template <typename RType>
        static float calcImpedance (RType& R)
        {
            constexpr float Ag = 100.0f; // op-amp gain
            constexpr float Ri = 10.0e6f; // op-amp input impedance
            constexpr float Ro = 1.0e-1f; // op-amp output impedance

            const auto [Ra, Rb, Rc] = R.getPortImpedances();
            const auto Rd = -(((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro) / (Ra * Rb + Ra * Rc + Rb * Rc + Rb * Ri + Ag * Rb * Ri + Rc * Ri - (Ra + Rb + Ri) * Ro));

            R.setSMatrixData ({ { (Ra * Rd * (Rb + Rc - Ro) + Rc * Ri * Ro + Rb * (Rc + Ri) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), (Ra * (-(Rc * Rd) + (Rc + Rd) * Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), (Ra * Rb * (Rd - Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Ra * Rb) / (Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri))) },
                                { (-(Rb * Rd * (Rc + Ag * Ri)) + Rb * (Rc + Rd) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Rc * Rd * (Ra + Ri) + (Ra * (Rb - Rd) - Rd * Ri + Rb * (Rc + Ri)) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro)), (Rb * (Ra + Ri) * (Rd - Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Rb * (Ra + Ri)) / (Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri))) },
                                { -((Rc * (Ag * Rd * Ri + Rb * (-Rd + Ro))) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro)), (Rc * Rd * (Ra + Ri + Ag * Ri) - Rc * (Ra + Ri) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), (Rc * Rd * (Ra + Rb + Ri) + Rb * (Ra + Ri) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Rc * (Ra + Rb + Ri)) / (Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri))) },
                                { (Rd * (Ag * (Rb + Rc) * Ri - Rb * Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Rd * (Ag * Rc * Ri + (Ra + Ri) * Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro)), -((Rd + Ro) / Ro), 0 } });

            return Rd;
        }
    };
    wdft::RtypeAdaptor<float, 3, ImpedanceCalc, decltype (Pa), decltype (Pb), decltype (Rd_C4)> R { Pa, Pb, Rd_C4 };

    // Port D
    wdft::ResistorCapacitorSeriesT<float> R6_C7 { 1.0e3f, 4.7e-6f };
    wdft::WDFSeriesT<float, decltype (R), decltype (R6_C7)> Sd { R, R6_C7 };

    wdft::DiodePairT<float, decltype (Sd)> diodes { Sd, 5.0e-9f, 25.85e-3f, 2.0f };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MouseDriveWDF)
};
