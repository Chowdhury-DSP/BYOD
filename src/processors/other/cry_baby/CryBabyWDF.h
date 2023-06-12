#pragma once

//#include "CryBabyWDFScatteringMatrix.h"
#include <pch.h>

struct CryBabyActiveStageWDF
{
    CryBabyActiveStageWDF() = default;

    void prepare (float fs)
    {
        Vin_R1_C1.prepare (fs);
        VR1_C5.prepare (fs);
        L1.prepare (fs);
        R8_C3.prepare (fs);
        C2_Vfb.prepare (fs);

        R3.setVoltage (9.0f);
    }

    inline float processSample (float Vin, float Vfb) noexcept
    {
        Vin_R1_C1.setVoltage (Vin);
        C2_Vfb.setVoltage (Vfb);

        const auto a_b = R.reflected();
        //        const auto b_b = -a_b; // 0.5f * std::tanh (std::exp (-a_b) - 2.0f); // Optimize me!
        const auto b_b = 5.0f * std::tanh (1.0f - std::exp (0.33333f * a_b));
        R.incident (b_b);

        return wdft::voltage<float> (VR1_C5);
    }

    // Port A
    wdft::ResistiveCapacitiveVoltageSourceT<float> Vin_R1_C1 { 68.0e3f, 0.01e-6f };

    // Port C
    wdft::ResistiveVoltageSourceT<float> R3 { 22.0e3f };
    wdft::ResistorCapacitorSeriesT<float> VR1_C5 { 100.0e3f, 0.22e-6f };
    wdft::WDFParallelT<float, decltype (R3), decltype (VR1_C5)> Pc { R3, VR1_C5 };

    // Port D
    wdft::ResistorT<float> R6 { 470.0e3f };

    // Port E
    wdft::InductorT<float> L1 { 1.0f };
    wdft::ResistorT<float> R7 { 33.0e3f };
    wdft::WDFParallelT<float, decltype (L1), decltype (R7)> Pe { L1, R7 };

    // Port F
    wdft::ResistorCapacitorParallelT<float> R8_C3 { 82.0e3f, 4.7e-6f };

    // Port G
    wdft::CapacitiveVoltageSourceT<float> C2_Vfb { 0.1e-6f };

    // Port H
    wdft::ResistorT<float> R2 { 1.5e3f };

    // R-Type Adaptor
    struct ImpedanceCalc
    {
        template <typename RType>
        static float calcImpedance (RType& R)
        {
            const auto [Ra, Rc, Rd, Re, Rf, Rg, Rh] = R.getPortImpedances();
            const auto Rb = (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) / (Rc * (Re + Rf) * Rg + Ra * (Rf * (Re + Rg) + Rc * (Re + Rf + Rg) + Rd * (Re + Rf + Rg)) + Rd * Rg * Rh + Rf * Rg * Rh + Rc * (Re + Rf + Rg) * Rh + Rd * Re * (Rg + Rh) + Rd * Rf * (Rg + Rh) + Re * Rf * (Rg + Rh));

            R.setSMatrixData ({ { (Rb * (Rc * (Re + Rf) * Rg + Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rc * (Re + Rf + Rg) * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) - Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Ra * (Rc * Re * Rg + Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rc * (Re + Rf + Rg) * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), (Ra * ((Rb + Re) * Rf * Rg + Rd * (Re + Rf) * Rg + Rf * (Re + Rg) * Rh + Rd * (Re + Rf + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Ra * (Rc * Re * Rg - Rb * Rf * Rg + Rc * (Re + Rf + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), (Ra * (Rc * Rd + Rb * (Rc + Rd + Rf)) * Rg - Ra * Rc * Rf * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Ra * (Rb * (Rc + Rd) * Rg + Rc * (Rd + Re) * Rg + Rc * (Re + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Ra * (Rb * (Rc + Rd) * Re + Rb * (Rc + Rd + Re) * Rf + Rc * Rd * (Re + Rf) + Rc * Rf * (Re + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Ra * (Rc * Re * Rf + Rb * Rf * (Re + Rg) + Rb * Rc * (Re + Rf + Rg) + Rb * Rd * (Re + Rf + Rg) + Rc * Rd * (Re + Rf + Rg))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))) },
                                { -((Rb * (Rc * Re * Rg + Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rc * (Re + Rf + Rg) * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), 0, (Rb * (Ra * Re * Rf + Rd * (Re + Rf) * Rg + Ra * Rd * (Re + Rf + Rg) + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Rb * (Ra * Rf * Rg + Rc * (Re + Rf) * Rg + Ra * Rc * (Re + Rf + Rg) + Rc * (Re + Rf + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), -((Rb * (Ra * Rc * Rf + Ra * (Rc + Rd + Rf) * Rg + Rc * Rf * (Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), (Rb * (Ra * Rc * Re - Ra * Rd * Rg + Rc * Re * Rg + Rc * (Re + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (-(Ra * Rb * ((Rc + Rd) * Re + (Rd + Re) * Rf)) + Rb * Rc * Rf * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rb * (Rc * Rf * Rg + Ra * (Rf * (Re + Rg) + Rc * (Re + Rf + Rg) + Rd * (Re + Rf + Rg)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) },
                                { (Rc * ((Rb + Re) * Rf * Rg + Rd * (Re + Rf) * Rg + Rf * (Re + Rg) * Rh + Rd * (Re + Rf + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rc * (Ra * Re * Rf + Rd * (Re + Rf) * Rg + Ra * Rd * (Re + Rf + Rg) + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), ((Rb - Rc) * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rb * (Rf * (Re + Rg) + Rd * (Re + Rf + Rg)) - Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Rc * (Rb * (Re + Rf) * Rg + Rb * (Re + Rf + Rg) * Rh + Ra * (Re * Rg + Rb * (Re + Rf + Rg) + (Re + Rf + Rg) * Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), -((Rc * (-(Ra * Rd * Rg) + Ra * Rf * (Rb + Rh) + Rb * Rf * (Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), (Rc * (Ra * Rb * Re + Ra * Rb * Rg + Ra * Rd * Rg + Ra * Re * Rg + Rb * Re * Rg + (Ra + Rb) * (Re + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rc * (Ra * Rd * (Re + Rf) + Rb * Rf * Rh + Ra * Rf * (Rb + Re + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Rc * (Ra * Re * Rf - Rb * Rf * Rg + Ra * Rd * (Re + Rf + Rg))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))) },
                                { -((Rd * (Rc * Re * Rg - Rb * Rf * Rg + Rc * (Re + Rf + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), -((Rd * (Ra * Rf * Rg + Rc * (Re + Rf) * Rg + Ra * Rc * (Re + Rf + Rg) + Rc * (Re + Rf + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), -((Rd * (Rb * (Re + Rf) * Rg + Rb * (Re + Rf + Rg) * Rh + Ra * (Re * Rg + Rb * (Re + Rf + Rg) + (Re + Rf + Rg) * Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), (-(Rb * Rd * ((Re + Rf) * Rg + (Re + Rf + Rg) * Rh)) + Rc * Rf * (Rg * Rh + Re * (Rg + Rh)) + Ra * (-(Rb * Rd * (Re + Rf + Rg)) + Rf * Rg * Rh + Rc * (Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * Re * (Rf + Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rd * (-((Rb + Rc) * Rf * (Rg + Rh)) - Ra * (Rb * Rf + Rc * (Rf + Rg) + Rf * (Rg + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rd * (Ra * (Re * (Rc + Rg) + Rb * (Re + Rg) + (Re + Rg) * Rh) + (Rb + Rc) * (Rg * Rh + Re * (Rg + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rd * (-(Ra * Rc * Re) + (Rb + Rc) * Rf * Rh + Ra * Rf * (Rb + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rd * (Ra * Rf * Rg + (Rb + Rc) * Rf * Rg + Ra * Rc * (Re + Rf + Rg))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) },
                                { (Re * (Rb * (Rc + Rd + Rf) * Rg + Rc * (Rd * Rg - Rf * Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Re * (Ra * Rc * Rf + Ra * (Rc + Rd + Rf) * Rg + Rc * Rf * (Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), -((Re * (-(Ra * Rd * Rg) + Ra * Rf * (Rb + Rh) + Rb * Rf * (Rg + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), -((Re * ((Rb + Rc) * Rf * (Rg + Rh) + Ra * (Rb * Rf + Rc * (Rf + Rg) + Rf * (Rg + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), (Rb * (Rd * Rf * Rg + Rf * Rg * Rh + Rd * (Rf + Rg) * Rh) - Rc * (Rd * Re * (Rg + Rh) + Re * Rf * (Rg + Rh) - Rb * (Rg * Rh + Rf * (Rg + Rh))) - Ra * (-(Rb * (Rf * Rg + Rd * (Rf + Rg))) + Re * (Rd + Rf) * (Rg + Rh) + Rc * (-(Rb * (Rf + Rg)) + Re * (Rd + Rf + Rg + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Re * ((Rc * Rd + Rb * (Rc + Rd)) * (Rg + Rh) + Ra * (Rb * (Rc + Rd) + Rd * (Rc + Rg) + (Rc + Rd) * Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), (Re * (Ra * Rc * Rd + Ra * Rb * (Rc + Rd + Rf) + Rc * (Rd + Rf) * Rh + Ra * (Rc + Rd + Rf) * Rh + Rb * (Rc + Rd + Rf) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Re * (Ra * Rc * Rf + Rc * (Rd + Rf) * Rg + Ra * (Rc + Rd + Rf) * Rg + Rb * (Rc + Rd + Rf) * Rg)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) },
                                { (Rf * (Rb * (Rc + Rd) * Rg + Rc * (Rd + Re) * Rg + Rc * (Re + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rf * (Ra * Rc * Re - Ra * Rd * Rg + Rc * Re * Rg + Rc * (Re + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rf * (Ra * Rb * Re + Ra * Rb * Rg + Ra * Rd * Rg + Ra * Re * Rg + Rb * Re * Rg + (Ra + Rb) * (Re + Rg) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rf * (Ra * (Re * (Rc + Rg) + Rb * (Re + Rg) + (Re + Rg) * Rh) + (Rb + Rc) * (Rg * Rh + Re * (Rg + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -((Rf * ((Rc * Rd + Rb * (Rc + Rd)) * (Rg + Rh) + Ra * (Rb * (Rc + Rd) + Rd * (Rc + Rg) + (Rc + Rd) * Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), (-(Rb * Rf * ((Rd + Re) * Rg + (Rd + Re + Rg) * Rh)) + Rc * (Rd * Rg * Rh + Rd * Re * (Rg + Rh) - Rb * Rf * (Rg + Rh)) + Ra * (-(Rb * Rf * (Rd + Re + Rg)) + Rd * Rg * Rh + Rd * Re * (Rg + Rh) + Rc * (-(Rb * Rf) + Rd * (Re + Rg) + Rg * Rh + Re * (Rg + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rf * (Ra * Rb * (Rc + Rd) + Ra * Rd * Rh + Rc * Rd * Rh + Rb * (Rc + Rd) * Rh + Ra * Rc * (Rd + Re + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rf * (Rc * Rd * Rg + Rb * (Rc + Rd) * Rg + Ra * (-(Rc * Re) + Rd * Rg))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) },
                                { (Rg * (Rb * (Rc + Rd) * Re + Rb * (Rc + Rd + Re) * Rf + Rc * Rd * (Re + Rf) + Rc * Rf * (Re + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (-(Ra * ((Rc + Rd) * Re + (Rd + Re) * Rf) * Rg) + Rc * Rf * Rg * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rg * (Ra * Rd * (Re + Rf) + Rb * Rf * Rh + Ra * Rf * (Rb + Re + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rg * (-(Ra * Rc * Re) + (Rb + Rc) * Rf * Rh + Ra * Rf * (Rb + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), ((Rc * (Rd + Rf) + Rb * (Rc + Rd + Rf)) * Rg * Rh + Ra * Rg * (Rc * Rd + Rb * (Rc + Rd + Rf) + (Rc + Rd + Rf) * Rh)) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rg * (Ra * Rb * (Rc + Rd) + Ra * Rd * Rh + Rc * Rd * Rh + Rb * (Rc + Rd) * Rh + Ra * Rc * (Rd + Re + Rh))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rc * (Re * Rf + Rd * (Re + Rf)) * Rh - Rb * Rg * (Rf * (Re + Rh) + Rc * (Re + Rf + Rh) + Rd * (Re + Rf + Rh)) + Ra * (-(Rb * (Rd + Rf) * Rg) + Re * Rf * Rh + Rd * (Re + Rf) * Rh + Rc * (Rd * (Re + Rf) - Rb * Rg + Rf * Rh + Re * (Rf + Rh)))) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), ((Ra * (Rc + Rd) * Re + Rb * (Rc + Rd) * Re + Rc * Re * Rf + Ra * (Rd + Re) * Rf + Rb * (Rc + Rd + Re) * Rf + Rc * Rd * (Re + Rf)) * Rg) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) },
                                { -(((Rc * Re * Rf + Rb * Rf * (Re + Rg) + Rb * Rc * (Re + Rf + Rg) + Rb * Rd * (Re + Rf + Rg) + Rc * Rd * (Re + Rf + Rg)) * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), ((Rc * Rf * Rg + Ra * (Rf * (Re + Rg) + Rc * (Re + Rf + Rg) + Rd * (Re + Rf + Rg))) * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), -(((Ra * Re * Rf - Rb * Rf * Rg + Ra * Rd * (Re + Rf + Rg)) * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh))))), ((Ra * Rf * Rg + (Rb + Rc) * Rf * Rg + Ra * Rc * (Re + Rf + Rg)) * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), ((Ra * Rc * Rf + (Rc * (Rd + Rf) + Ra * (Rc + Rd + Rf) + Rb * (Rc + Rd + Rf)) * Rg) * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), ((Rc * Rd * Rg + Rb * (Rc + Rd) * Rg + Ra * (-(Rc * Re) + Rd * Rg)) * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), ((Ra * (Rc + Rd) * Re + Rb * (Rc + Rd) * Re + Rc * Re * Rf + Ra * (Rd + Re) * Rf + Rb * (Rc + Rd + Re) * Rf + Rc * Rd * (Re + Rf)) * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))), (Rc * (Re * Rf + Rd * (Re + Rf)) * Rg + Ra * (Re * Rf * Rg + Rd * (Re + Rf) * Rg + Rc * Re * (Rf + Rg) + Rc * Rd * (Re + Rf + Rg)) - Rb * (Rf * (Re + Rg) + Rc * (Re + Rf + Rg) + Rd * (Re + Rf + Rg)) * Rh) / (Rc * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh)) + Ra * (Rd * (Re + Rf) * Rg + Rf * Rg * Rh + Rd * (Re + Rf + Rg) * Rh + Re * Rf * (Rg + Rh) + Rc * (Rd * (Re + Rf + Rg) + (Rf + Rg) * Rh + Re * (Rf + Rg + Rh)))) } });
            return Rb;
        }
    };
    wdft::RtypeAdaptor<float,
                       1,
                       ImpedanceCalc,
                       decltype (Vin_R1_C1),
                       decltype (Pc),
                       decltype (R6),
                       decltype (Pe),
                       decltype (R8_C3),
                       decltype (C2_Vfb),
                       decltype (R2)>
        R { Vin_R1_C1, Pc, R6, Pe, R8_C3, C2_Vfb, R2 };
};

struct CryBabyOutputStageWDF
{
    CryBabyOutputStageWDF() = default;

    void prepare (float fs)
    {
        C4.prepare (fs);
        C5.prepare (fs);
    }

    inline auto processSample (float Vi) noexcept
    {
        Vin.setVoltage (Vi);

        Vin.incident (R.reflected());
        R.incident (Vin.reflected());

        const auto Vout = wdft::voltage<float> (VR1m) + wdft::voltage<float> (VR1p);
        const auto Vfb = wdft::voltage<float> (RL);
        return std::make_pair (Vout, Vfb);
    }

    // Port B
    wdft::ResistorT<float> R5 { 470.0e3f };

    // Port C
    wdft::CapacitorT<float> C5 { 0.22e-6f };
    wdft::ResistorT<float> VR1p { 50.0e3f };
    wdft::WDFSeriesT<float, decltype (C5), decltype (VR1p)> Sc { C5, VR1p };

    // Port D
    wdft::CapacitorT<float> C4 { 0.22e-6f };

    // Port E
    wdft::ResistorT<float> VR1m { 50.0e3f };

    // Port F
    wdft::ResistorT<float> RL { 2.0e6f };

    // R-Type Adaptor
    struct ImpedanceCalc
    {
        template <typename RType>
        static float calcImpedance (RType& R)
        {
            const auto [Rb, Rc, Rd, Re, Rf] = R.getPortImpedances();
            const auto Ra = (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)) / (Rd * (Re + Rf) + Rb * (Rd + Re + Rf) + Rc * (Rd + Re + Rf));

            R.setSMatrixData ({ { 0, -((Ra * (Rd * Re + Rc * (Rd + Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf))), -((Ra * (Rd * Rf + Rb * (Rd + Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf))), (Ra * (-(Rb * Re) + Rc * Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Ra * (Rb * Rd + (Rb + Rc + Rd) * Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Ra * (Rc * Rd + (Rb + Rc + Rd) * Re)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)) },
                                { -((Rb * (Rd * Re + Rc * (Rd + Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf))), (-(Rb * (Re * (Rd + Rf) + Rc * (Rd + Re + Rf))) + Ra * (Rd * (Re + Rf) + Rc * (Rd + Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Rb * (Re * Rf + Ra * (Rd + Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Rb * (Ra * Re + (Ra + Rc + Re) * Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (-(Ra * Rb * Rd) + Rb * Rc * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Rb * ((Ra + Rc) * Rd + (Rc + Rd) * Re)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)) },
                                { -((Rc * (Rd * Rf + Rb * (Rd + Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf))), (Rc * (Re * Rf + Ra * (Rd + Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Rd * Re * Rf + Rb * Re * (Rd + Rf) - Ra * Rc * (Rd + Re + Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), -((Rc * (Re * (Rb + Rf) + Ra * (Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf))), (Rc * ((Ra + Rb) * Rd + (Rb + Rd) * Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (-(Ra * Rc * Rd) + Rb * Rc * Re) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)) },
                                { (-(Rb * Rd * Re) + Rc * Rd * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Rd * (Ra * Re + (Ra + Rc + Re) * Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), -((Rd * (Re * (Rb + Rf) + Ra * (Re + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf))), (-(Rb * Rd * (Rc + Re)) - Rd * (Rc + Re) * Rf + Ra * Rb * (Re + Rf) + Ra * Rc * (Re + Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), -((Rd * (Ra * (Rb + Rc) + Rc * (Rb + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf))), (Rd * (Ra * (Rb + Rc) + Rb * (Rc + Re))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)) },
                                { (Re * (Rb * Rd + (Rb + Rc + Rd) * Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (-(Ra * Rd * Re) + Rc * Re * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Re * ((Ra + Rb) * Rd + (Rb + Rd) * Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), -((Re * (Ra * (Rb + Rc) + Rc * (Rb + Rf))) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf))), (-(Ra * Rb * Re) - Ra * (Rc + Rd) * Re + Rc * Rd * Rf + Rb * Rc * (Rd + Rf)) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), ((Rb * Rc + Ra * (Rb + Rc + Rd)) * Re) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)) },
                                { ((Rc * Rd + (Rb + Rc + Rd) * Re) * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (((Ra + Rc) * Rd + (Rc + Rd) * Re) * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (-(Ra * Rd * Rf) + Rb * Re * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), ((Ra * (Rb + Rc) + Rb * (Rc + Re)) * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), ((Rb * Rc + Ra * (Rb + Rc + Rd)) * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)), (Rb * Rd * Re + Rb * Rc * (Rd + Re) - Ra * (Rb + Rc + Rd) * Rf) / (Rd * Re * Rf + Rc * (Rd + Re) * Rf + Rb * Re * (Rd + Rf) + Rb * Rc * (Rd + Re + Rf)) } });
            return Ra;
        }
    };
    wdft::RtypeAdaptor<float,
                       0,
                       ImpedanceCalc,
                       decltype (R5),
                       decltype (Sc),
                       decltype (C4),
                       decltype (VR1m),
                       decltype (RL)>
        R { R5, Sc, C4, VR1m, RL };

    // Port A
    wdft::IdealVoltageSourceT<float, decltype (R)> Vin { R };
};

struct CryBabyWDF
{
    CryBabyWDF() = default;

    void prepare (float fs)
    {
        activeStage.prepare (fs);
        outputStage.prepare (fs);
        z1 = 0.0f;
    }

    void setWahAmount (float wah01)
    {
        outputStage.VR1p.setResistanceValue ((1.0f - wah01) * 100.0e3f);
        outputStage.VR1m.setResistanceValue (wah01 * 100.0e3f);
    }

    inline float processSample (float x) noexcept
    {
        const auto Vb = activeStage.processSample (x, z1);
        float y;
        std::tie (y, z1) = outputStage.processSample (Vb);
        return y;
    }

    CryBabyActiveStageWDF activeStage;
    CryBabyOutputStageWDF outputStage;

    float z1 = 0.0f;
};
