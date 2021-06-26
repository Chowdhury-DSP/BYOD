#pragma once

#include <pch.h>

template <template <typename, typename, chowdsp::WDFT::DiodeQuality> typename DiodeType>
class DiodeClipperWDF
{
public:
    DiodeClipperWDF (float sampleRate) : C1 (capVal, sampleRate) {}

    static float getDiodeIs (int diodeType)
    {
        switch (diodeType)
        {
        case 0: // GZ34
            return 2.52e-9f;
        case 1: // 1N34
            return 15.0e-6f;
        case 2: // 1N4148
            return 2.64e-9f;
        }

        jassertfalse;
        return 1.0e-9f;
    }

    void setParameters (float cutoff, float diodeIs, float nDiodes)
    {
        Vs.setResistanceValue (1.0f / (MathConstants<float>::twoPi * cutoff * capVal));
        dp.setDiodeParameters (diodeIs, 0.02585f, nDiodes);
    }

    void process (float* buffer, const int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
        {
            Vs.setVoltage (buffer[n]);

            dp.incident (P1.reflected());
            buffer[n] = chowdsp::WDFT::voltage<float> (C1);
            P1.incident (dp.reflected());
        }
    }

private:
    static constexpr float capVal = 47.0e-9f;
    using wdf_type = float;
    using Res = chowdsp::WDFT::ResistorT<wdf_type>;
    using Cap = chowdsp::WDFT::CapacitorT<wdf_type>;
    using ResVs = chowdsp::WDFT::ResistiveVoltageSourceT<wdf_type>;

    ResVs Vs { 4700.0f };
    Cap C1;

    chowdsp::WDFT::WDFParallelT<wdf_type, Cap, ResVs> P1 { C1, Vs };
    DiodeType<wdf_type, decltype (P1), chowdsp::WDFT::DiodeQuality::Best> dp { P1, 2.52e-9f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiodeClipperWDF)
};
