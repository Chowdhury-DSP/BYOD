#pragma once

#include <pch.h>

namespace gain_stage
{
using namespace chowdsp::wdft;
struct FeedForward2WDF
{
    FeedForward2WDF() = default;

    void prepare (float sample_rate, int samples_per_block)
    {
        R5_C4.prepare (sample_rate);
        R9_C6.prepare (sample_rate);
        R15_C11.prepare (sample_rate);
        R18_C12.prepare (sample_rate);

        RVTop_smooth.mappingFunction = [] (float gain)
        {
            return gain * 100.0e3f;
        };
        RVTop_smooth.setRampLength (0.05);
        RVTop_smooth.prepare ((double) sample_rate, samples_per_block);
    }

    inline float processSample (float x)
    {
        Vin.setVoltage (x);
        Vin.incident (I1.reflected());
        I1.incident (Vin.reflected());
        auto y = current<float> (R16);
        return (float) y;
    }

    void process (nonstd::span<float> buffer, float gain_param) noexcept
    {
        RVTop_smooth.process (gain_param, (int) buffer.size());
        if (RVTop_smooth.isSmoothing())
        {
            const auto RVTop_smooth_data = RVTop_smooth.getSmoothedBuffer();
            for (auto [n, sample] : chowdsp::enumerate (buffer))
            {
                RVTop.setResistanceValue (std::max (RVTop_smooth_data[n], 1.0f));
                RVBot.setResistanceValue (std::max (100.0e3f - RVTop_smooth_data[n], 1.0f));

                Vin.setVoltage (sample);
                Vin.incident (I1.reflected());
                I1.incident (Vin.reflected());
                sample = current<float> (R16);
            }
        }
        else
        {
            RVTop.setResistanceValue (std::max (RVTop_smooth.getCurrentValue(), 1.0f));
            RVBot.setResistanceValue (std::max (100.0e3f - RVTop_smooth.getCurrentValue(), 1.0f));
            for (auto& sample : buffer)
            {
                Vin.setVoltage (sample);
                Vin.incident (I1.reflected());
                I1.incident (Vin.reflected());
                sample = current<float> (R16);
            }
        }
    }

    chowdsp::SmoothedBufferValue<float> RVTop_smooth;

    ResistorCapacitorSeriesT<float> R18_C12 { 12.0e3f, 27e-9f };
    ResistorT<float> R17 { 27.0e3f };
    WDFParallelT<float, decltype (R18_C12), decltype (R17)> P1 { R18_C12, R17 };

    ResistorCapacitorSeriesT<float> R15_C11 { 22.0e3f, 2.2e-9f };
    ResistorT<float> R16 { 47.0e3f };
    WDFSeriesT<float, decltype (R15_C11), decltype (R16)> S3 { R15_C11, R16 };

    WDFParallelT<float, decltype (S3), decltype (P1)> P2 { S3, P1 };
    ResistorT<float> RVTop { 50.0e3f };
    ResistorT<float> RVBot { 50.0e3f };
    WDFParallelT<float, decltype (P2), decltype (RVBot)> P3 { P2, RVBot };
    WDFSeriesT<float, decltype (P3), decltype (RVTop)> S4 { P3, RVTop };

    ResistorCapacitorSeriesT<float> R9_C6 { 1.0e3f, 390.0e-9f };
    WDFParallelT<float, decltype (S4), decltype (R9_C6)> P4 { S4, R9_C6 };
    ResistorT<float> R8 { 1.5e3f };
    WDFParallelT<float, decltype (P4), decltype (R8)> P5 { P4, R8 };
    ResistiveVoltageSourceT<float> Vbias;
    WDFSeriesT<float, decltype (P5), decltype (Vbias)> S6 { P5, Vbias };

    ResistorCapacitorParallelT<float> R5_C4 { 5.1e3f, 68e-9f };
    WDFSeriesT<float, decltype (R5_C4), decltype (S6)> S7 { R5_C4, S6 };
    PolarityInverterT<double, decltype (S7)> I1 { S7 };

    IdealVoltageSourceT<double, decltype (I1)> Vin { I1 };
};
} // namespace gain_stage
