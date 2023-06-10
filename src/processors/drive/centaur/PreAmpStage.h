#pragma once

#include <pch.h>

namespace gain_stage
{
using namespace chowdsp::wdft;
struct PreAmpWDF
{
    void prepare (float sample_rate, int max_block_size)
    {
        C5.prepare (sample_rate);
        C3.prepare (sample_rate);
        C16.prepare (sample_rate);

        Vbias.setVoltage (0.0f);
        Vbias2.setVoltage (0.0f);

        Rbias_smooth.mappingFunction = [] (float x)
        { return 100.0e3f * x; };
        Rbias_smooth.setRampLength (0.05);
        Rbias_smooth.prepare ((double) sample_rate, max_block_size);
    }

    void process (nonstd::span<float> buffer, nonstd::span<float> ff1_buffer, float gain_param) noexcept
    {
        Rbias_smooth.process (gain_param, (int) buffer.size());

        if (Rbias_smooth.isSmoothing())
        {
            const auto Rbias_smooth_data = Rbias_smooth.getSmoothedBuffer();
            for (auto [n, sample] : chowdsp::enumerate (buffer))
            {
                Vbias.setResistanceValue (Rbias_smooth_data[n]);
                Vin.setVoltage (sample);
                Vin.incident (I1.reflected());
                sample = voltage<float> (Vbias) + voltage<float> (R6);
                I1.incident (Vin.reflected());
                ff1_buffer[n] = current<float> (Vbias2);
            }
        }
        else
        {
            Vbias.setResistanceValue (Rbias_smooth.getCurrentValue());
            for (auto [n, sample] : chowdsp::enumerate (buffer))
            {
                Vin.setVoltage (sample);
                Vin.incident (I1.reflected());
                sample = voltage<float> (Vbias) + voltage<float> (R6);
                I1.incident (Vin.reflected());
                ff1_buffer[n] = current<float> (Vbias2);
            }
        }
    }

    chowdsp::SmoothedBufferValue<float> Rbias_smooth;

    ResistorT<float> R6 { 10.0e3f };
    CapacitorT<float> C5 { 68.0e-9f };
    WDFParallelT<float, decltype (R6), decltype (C5)> P1 { R6, C5 };
    ResistiveVoltageSourceT<float> Vbias;
    WDFSeriesT<float, decltype (P1), decltype (Vbias)> S1 { P1, Vbias };

    ResistiveVoltageSourceT<float> Vbias2 { 15.0e3f };
    CapacitorT<float> C16 { 1.0e-6f };
    WDFParallelT<float, decltype (Vbias2), decltype (C16)> P2 { Vbias2, C16 };

    ResistorT<float> R7 { 1.5e3f };
    WDFSeriesT<float, decltype (P2), decltype (R7)> S2 { P2, R7 };
    WDFParallelT<float, decltype (S1), decltype (S2)> P3 { S1, S2 };

    CapacitorT<float> C3 { 0.1e-6f };
    WDFSeriesT<float, decltype (P3), decltype (C3)> S3 { P3, C3 };
    PolarityInverterT<float, decltype (S3)> I1 { S3 };

    IdealVoltageSourceT<float, decltype (I1)> Vin { I1 };
};
} // namespace gain_stage
