#pragma once

#include <pch.h>

namespace ZenDriveWDFs
{
using Res = wdft::ResistorT<float>;
using ResVs = wdft::ResistiveVoltageSourceT<float>;
using ResIs = wdft::ResistiveCurrentSourceT<float>;
using Cap = wdft::CapacitorT<float>;

class InputBufferWDF
{
public:
    InputBufferWDF (float sampleRate) : C3 (47.0e-9f, sampleRate)
    {
        R4.setVoltage (4.5f);
    }

    inline float process (float V_in) noexcept
    {
        Vin.setVoltage (V_in);
        Vin.incident (I1.reflected());
        I1.incident (Vin.reflected());
        return wdft::voltage<float> (R4);
    }

private:
    ResVs R4 { 470.0e3f };
    Cap C3;
    
    wdft::WDFSeriesT<float, ResVs, Cap> S1 { R4, C3 };
    wdft::PolarityInverterT<float, decltype (S1)> I1 { S1 };

    wdft::IdealVoltageSourceT<float, decltype (I1)> Vin { I1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputBufferWDF)
};

class VoiceCircuitWDF
{
public:
    VoiceCircuitWDF (float sampleRate) : C5 (100.0e-9f, sampleRate)
    {
        Vref.setVoltage (4.5f);
    }

    void setVoiceParameter (float voiceParam)
    {
        R6.setResistanceValue (voiceParam * R6Val);
    }

    inline float process (float Vplus) noexcept
    {
        R5.setVoltage (Vplus);
        Vref.incident (S2.reflected());
        S2.incident (Vref.reflected());
        return wdft::current<float> (R5);
    }

private:
    static constexpr auto R6Val = 10.0e3f;
    ResVs R5 { 1.0e3f };
    Res R6 { R6Val };
    Cap C5;

    wdft::PolarityInverterT<float, ResVs> I1 { R5 };
    wdft::WDFSeriesT<float, Res, decltype (I1)> S1 { R6, I1 };
    wdft::WDFSeriesT<float, Cap, decltype (S1)> S2 { C5, S1 };

    wdft::IdealVoltageSourceT<float, decltype (S2)> Vref { S2 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceCircuitWDF)
};

class DriveStageWDF
{
public:
    DriveStageWDF (float sampleRate) : C5 (100.0e-12f, sampleRate)
    {
    }

    void setGainParam (float gainParam)
    {
        Rv9.setResistanceValue (Rv9Val * gainParam);
    }

    inline float process (float I0) noexcept
    {
        Rv9.setCurrent (I0);

        diodes.incident (P1.reflected());
        P1.incident (diodes.reflected());

        return wdft::voltage<float> (C5);
    }

private:
    static constexpr auto Rv9Val = 500.0e3f;
    ResIs Rv9 { Rv9Val };
    Cap C5;

    wdft::WDFParallelT<float, Cap, ResIs> P1 { C5, Rv9 };
    wdft::DiodePairT<float, decltype (P1)> diodes { P1, 5.241435962608312e-10f, 0.07877217375325735f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriveStageWDF)
};
} // namespace ZenDriveWDFs

class ZenDriveWDF
{
public:
    ZenDriveWDF (float sampleRate) : inputBuffer (sampleRate),
                                     voiceCircuit (sampleRate),
                                     driveStage (sampleRate)
    {}

    void setParameters (float voiceParam, float gainParam)
    {
        voiceCircuit.setVoiceParameter (voiceParam);
        driveStage.setGainParam (gainParam);
    }

    void process (float* buffer, const int numSamples)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            auto Vplus = inputBuffer.process (buffer[n]);
            auto I0 = voiceCircuit.process (Vplus);
            auto Vf = driveStage.process (I0);
            buffer[n] = Vf + Vplus;
        }
    }

private:
    ZenDriveWDFs::InputBufferWDF inputBuffer;
    ZenDriveWDFs::VoiceCircuitWDF voiceCircuit;
    ZenDriveWDFs::DriveStageWDF driveStage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZenDriveWDF)
};
