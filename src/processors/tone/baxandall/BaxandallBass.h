#pragma once

#include <pch.h>

class BaxandallBass
{
public:
    BaxandallBass() = default;

    void prepare (double sampleRate)
    {
        C1.prepare ((float) sampleRate);
        C2.prepare ((float) sampleRate);
        C4.prepare ((float) sampleRate);
    }

    void reset()
    {
        C1.reset();
        C2.reset();
        C4.reset();
    }

    void setBass (float bass)
    {
        bass = jlimit (0.001f, 0.999f, bass);
        VR1b.setResistanceValue (100000.0f * (1.0f - bass));
        VR1a.setResistanceValue (100000.0f * bass);
    }

    void setTreble (float treble)
    {
        treble = jlimit (0.001f, 0.999f, treble);
        VR2b.setResistanceValue (100000.0f * treble);
    }

    inline float processSample (float x) noexcept
    {
        Vin.setVoltage (x);

        Vin.incident (S5.reflected());
        S5.incident (Vin.reflected());

        return wdft::voltage<float> (C4) + wdft::voltage<float> (VR2b);
    }

private:
    wdft::ResistorT<float> R1 { 10000.0f };
    wdft::ResistorT<float> R2 { 1000.0f };
    wdft::ResistorT<float> R3 { 10000.0f };

    wdft::ResistorT<float> VR1a { 100000.0f };
    wdft::ResistorT<float> VR1b { 100000.0f };
    wdft::ResistorT<float> VR2b { 100000.0f };

    wdft::CapacitorT<float> C1 { 22.0e-9f };
    wdft::CapacitorT<float> C2 { 220.0e-9f };
    wdft::CapacitorT<float> C4 { 22.0e-9f };

    wdft::WDFParallelT<float, decltype (C1), decltype (VR1a)> P1 { C1, VR1a };
    wdft::WDFParallelT<float, decltype (C2), decltype (VR1b)> P2 { C2, VR1b };

    wdft::WDFParallelT<float, decltype (C4), decltype (VR2b)> S1 { C4, VR2b };
    wdft::WDFParallelT<float, decltype (S1), decltype (R3)> S2 { S1, R3 };

    wdft::WDFSeriesT<float, decltype (P1), decltype (R2)> S3 { P1, R2 };
    wdft::WDFParallelT<float, decltype (S3), decltype (S2)> P3 { S3, S2 };
    wdft::WDFSeriesT<float, decltype (P3), decltype (P2)> S4 { P3, P2 };
    wdft::WDFSeriesT<float, decltype (S4), decltype (R1)> S5 { S4, R1 };

    wdft::IdealVoltageSourceT<float, decltype (S5)> Vin { S5 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaxandallBass)
};
