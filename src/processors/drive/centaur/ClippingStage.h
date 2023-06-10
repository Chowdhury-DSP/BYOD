#pragma once

#include <pch.h>

namespace gain_stage
{
using namespace chowdsp::wdft;
struct ClippingStageWDF
{
    ClippingStageWDF() = default;

    void prepare (float sample_rate)
    {
        C9.prepare (sample_rate);
        C10.prepare (sample_rate);
        Vbias.setVoltage (0.0f);
    }

    void process (nonstd::span<float> buffer) noexcept
    {
        for (auto& sample : buffer)
        {
            Vin.setVoltage (sample);
            D23.incident (P1.reflected());
            P1.incident (D23.reflected());
            sample = current<float> (C10);
        }
    }

    ResistiveVoltageSourceT<float> Vin;
    CapacitorT<float> C9 { 1.0e-6f };
    ResistorT<float> R13 { 1000.0f };

    CapacitorT<float> C10 { 1.0e-6f };
    ResistiveVoltageSourceT<float> Vbias { 47000.0f };

    PolarityInverterT<float, decltype (Vin)> I1 { Vin };
    WDFSeriesT<float, decltype (I1), decltype (C9)> S1 { I1, C9 };
    WDFSeriesT<float, decltype (S1), decltype (R13)> S2 { S1, R13 };

    WDFSeriesT<float, decltype (C10), decltype (Vbias)> S3 { C10, Vbias };
    WDFParallelT<float, decltype (S2), decltype (S3)> P1 { S2, S3 };

    DiodePairT<float, decltype (P1)> D23 { P1, 15e-6f, 0.02585f };
};
} // namespace gain_stage
