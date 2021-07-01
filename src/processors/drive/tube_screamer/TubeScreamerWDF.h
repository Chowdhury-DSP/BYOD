#pragma once

#include <pch.h>

namespace TubeScreamerWDFs
{
using Res = wdft::ResistorT<float>;
using ResVs = wdft::ResistiveVoltageSourceT<float>;
using ResIs = wdft::ResistiveCurrentSourceT<float>;
using Cap = wdft::CapacitorT<float>;

class InputBufferWDF
{
public:
    InputBufferWDF (float sampleRate) : C2 (1.0e-6f, sampleRate)
    {
        R5.setVoltage (4.5f);
    }

    inline float process (float V_in) noexcept
    {
        Vin.setVoltage (V_in);
        Vin.incident (S1.reflected());
        S1.incident (Vin.reflected());
        return wdft::voltage<float> (R5);
    }

private:
    ResVs R5 { 10.0e3f };
    Cap C2;

    wdft::WDFSeriesT<float, ResVs, Cap> S1 { R5, C2 };

    wdft::IdealVoltageSourceT<float, decltype (S1)> Vin { S1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputBufferWDF)
};

class VplusWDF
{
public:
    VplusWDF (float sampleRate) : C3 (0.047e-6f, sampleRate)
    {
    }

    inline float process (float Vplus) noexcept
    {
        Vin.setVoltage (Vplus);
        Vin.incident (S1.reflected());
        S1.incident (Vin.reflected());
        return wdft::current<float> (C3);
    }

private:
    Res R4 { 4.7e3f };
    Cap C3;

    wdft::WDFSeriesT<float, Res, Cap> S1 { R4, C3 };

    wdft::IdealVoltageSourceT<float, decltype (S1)> Vin { S1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VplusWDF)
};

class DriveStageWDF
{
public:
    DriveStageWDF (float sampleRate) : C4 (51.0e-12f, sampleRate)
    {
    }

    void setGainParam (float gainParam)
    {
        Rv6.setResistanceValue (P1Val * gainParam + R6Val);
    }

    void setDiodeParams (float nDiodes, float diodeIs)
    {
        dp.setDiodeParameters (diodeIs, Vt, nDiodes);
    }

    inline float process (float I0) noexcept
    {
        Rv6.setCurrent (I0);

        dp.incident (P1.reflected());
        P1.incident (dp.reflected());

        return wdft::voltage<float> (C4);
    }

private:
    static constexpr float Vt = 0.02585f;
    static constexpr auto P1Val = 500.0e3f;
    static constexpr auto R6Val = 51.0e3f;

    ResIs Rv6 { P1Val + R6Val };
    Cap C4;

    wdft::WDFParallelT<float, Cap, ResIs> P1 { C4, Rv6 };
    wdft::DiodePairT<float, decltype (P1), wdft::DiodeQuality::Best> dp { P1, 2.52e-9f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriveStageWDF)
};
} // namespace TubeScreamerWDFs

class TubeScreamerWDF
{
public:
    TubeScreamerWDF (float sampleRate) : inputBuffer (sampleRate),
                                         vplusCircuit (sampleRate),
                                         driveStage (sampleRate)
    {
        nDiodesSmooth.reset ((double) sampleRate, 0.01);
        gainSmooth.reset ((double) sampleRate, 0.01);
    }

    void setParameters (float gainParam, float diodeIs, float nDiodes, bool force = false)
    {
        curDiodeIs = diodeIs;
        if (force)
        {
            nDiodesSmooth.setCurrentAndTargetValue (nDiodes);
            gainSmooth.setCurrentAndTargetValue (gainParam);

            driveStage.setDiodeParams (nDiodesSmooth.getTargetValue(), curDiodeIs);
            driveStage.setGainParam (gainSmooth.getTargetValue());
        }
        else
        {
            nDiodesSmooth.setTargetValue (nDiodes);
            gainSmooth.setTargetValue (gainParam);
        }
    }

    inline float processSample (float x) noexcept
    {
        auto Vplus = inputBuffer.process (x);
        auto I0 = vplusCircuit.process (Vplus);
        auto Vf = driveStage.process (I0);
        return Vf + Vplus;
    }

    void process (float* buffer, const int numSamples)
    {
        if (nDiodesSmooth.isSmoothing() && gainSmooth.isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                driveStage.setDiodeParams (nDiodesSmooth.getNextValue(), curDiodeIs);
                driveStage.setGainParam (gainSmooth.getNextValue());

                buffer[n] = processSample (buffer[n]);
            }
            return;
        }

        if (nDiodesSmooth.isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                driveStage.setDiodeParams (nDiodesSmooth.getNextValue(), curDiodeIs);
                buffer[n] = processSample (buffer[n]);
            }
            return;
        }

        if (gainSmooth.isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                driveStage.setGainParam (gainSmooth.getNextValue());
                buffer[n] = processSample (buffer[n]);
            }
            return;
        }

        for (int n = 0; n < numSamples; ++n)
            buffer[n] = processSample (buffer[n]);
    }

private:
    TubeScreamerWDFs::InputBufferWDF inputBuffer;
    TubeScreamerWDFs::VplusWDF vplusCircuit;
    TubeScreamerWDFs::DriveStageWDF driveStage;

    SmoothedValue<float, ValueSmoothingTypes::Linear> nDiodesSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> gainSmooth;
    float curDiodeIs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeScreamerWDF)
};
