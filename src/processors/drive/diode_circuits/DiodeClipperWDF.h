#pragma once

#include <pch.h>

template <template <typename, typename, wdft::DiodeQuality> typename DiodeType>
class DiodeClipperWDF
{
public:
    DiodeClipperWDF (float sampleRate) : C1 (capVal, sampleRate)
    {
        cutoffSmooth.reset ((double) sampleRate, 0.01);
        nDiodesSmooth.reset ((double) sampleRate, 0.01);
    }

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

    void setParameters (float cutoff, float diodeIs, float nDiodes, bool force = false)
    {
        curDiodeIs = diodeIs;
        if (force)
        {
            cutoffSmooth.setCurrentAndTargetValue (cutoff);
            nDiodesSmooth.setCurrentAndTargetValue (nDiodes);

            Vs.setResistanceValue (1.0f / (MathConstants<float>::twoPi * cutoffSmooth.getNextValue() * capVal));
            dp.setDiodeParameters (curDiodeIs, Vt, nDiodesSmooth.getNextValue());
        }
        else
        {
            cutoffSmooth.setTargetValue (cutoff);
            nDiodesSmooth.setTargetValue (nDiodes);
        }
    }

    inline float processSample (float x) noexcept
    {
        Vs.setVoltage (x);
        
        dp.incident (P1.reflected());
        auto y = wdft::voltage<float> (C1);
        P1.incident (dp.reflected());
        
        return y;
    }

    void process (float* buffer, const int numSamples) noexcept
    {
        if (cutoffSmooth.isSmoothing() && nDiodesSmooth.isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                Vs.setResistanceValue (1.0f / (MathConstants<float>::twoPi * cutoffSmooth.getNextValue() * capVal));
                dp.setDiodeParameters (curDiodeIs, Vt, nDiodesSmooth.getNextValue());

                buffer[n] = processSample (buffer[n]);
            }
            return;
        }
        
        if (cutoffSmooth.isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                Vs.setResistanceValue (1.0f / (MathConstants<float>::twoPi * cutoffSmooth.getNextValue() * capVal));
                buffer[n] = processSample (buffer[n]);
            }
            return;
        }
        
        if (nDiodesSmooth.isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                dp.setDiodeParameters (curDiodeIs, Vt, nDiodesSmooth.getNextValue());
                buffer[n] = processSample (buffer[n]);
            }
            return;
        }

        for (int n = 0; n < numSamples; ++n)
            buffer[n] = processSample (buffer[n]);
    }

private:
    static constexpr float Vt = 0.02585f;
    static constexpr float capVal = 47.0e-9f;
    using wdf_type = float;
    using Res = wdft::ResistorT<wdf_type>;
    using Cap = wdft::CapacitorT<wdf_type>;
    using ResVs = wdft::ResistiveVoltageSourceT<wdf_type>;

    ResVs Vs { 4700.0f };
    Cap C1;

    wdft::WDFParallelT<wdf_type, Cap, ResVs> P1 { C1, Vs };
    DiodeType<wdf_type, decltype (P1), wdft::DiodeQuality::Best> dp { P1, 2.52e-9f };

    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> cutoffSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> nDiodesSmooth;
    float curDiodeIs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiodeClipperWDF)
};
