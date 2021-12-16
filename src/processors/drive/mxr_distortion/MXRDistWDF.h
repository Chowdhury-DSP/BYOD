#pragma once

#include <pch.h>

namespace MXRDistWDFs
{
using Res = wdft::ResistorT<float>;
using ResVs = wdft::ResistiveVoltageSourceT<float>;
using ResIs = wdft::ResistiveCurrentSourceT<float>;
using Cap = wdft::CapacitorT<float>;

class MXRDistPlus1
{
private:
    ResVs Vres;
    ResVs Vb { 1.0e6f }; //encompasses R2

    Res R1 { 10.0e3f };
    Res R4 { 1.0e6f };

    static constexpr float R3Val = 4.7e3f;
    static constexpr float rDistVal = 1.0e6f;
    Res ResDist_R3 { rDistVal + R3Val }; //distortion potentiometer

    Cap C1 { 1.0e-9f };
    Cap C2 { 10.0e-9f };
    Cap C3 { 47.0e-9f };
    Cap C4 { 1.0e-6f };

    wdft::WDFSeriesT<float, Cap, Res> S3 { C2, R1 };
    wdft::WDFParallelT<float, ResVs, Cap> P1 { Vres, C1 };
    wdft::WDFSeriesT<float, decltype (S3), decltype (P1)> S4 { S3, P1 };
    wdft::WDFParallelT<float, decltype (S4), ResVs> P2 { S4, Vb };
    wdft::WDFSeriesT<float, Res, Cap> S2 { ResDist_R3, C3 };

    struct ImpedanceCalc
    {
        template <typename RType>
        static void calcImpedance (RType& R)
        {
            const auto [_, Rb, Rc, Rd] = R.getPortImpedances();

            R.setSMatrixData ({ { 1.0f, 0.0f, 0.0f, 0.0f },
                                { (200.0f * Rb) / (101.0f * Rb + Rc), 1.0f - (202.0f * Rb) / (101.0f * Rb + Rc), (2.0f * Rb) / (101.0f * Rb + Rc), 0.0f },
                                { -(200.0f * Rc) / (101.0f * Rb + Rc), (202.0f * Rc) / (101.0f * Rb + Rc), 1.0f - (2.0f * Rc) / (101.0f * Rb + Rc), 0.0f },
                                { (200.0f * Rd * (Rb + Rc)) / (101.0f * Rb * Rd + Rc * Rd), -(200.0f * Rc * Rd) / (101.0f * Rb * Rd + Rc * Rd), -(200.0f * Rb * Rd) / (101.0f * Rb * Rd + Rc * Rd), -1.0f } });
        }
    };

    wdft::RootRtypeAdaptor<float, ImpedanceCalc, decltype (P2), decltype (S2), Res, Cap> R { std::tie (P2, S2, R4, C4) };

public:
    MXRDistPlus1() = default;

    void prepare (float sampleRate)
    {
        C1.prepare (sampleRate);
        C2.prepare (sampleRate);
        C3.prepare (sampleRate);
        C4.prepare (sampleRate);

        Vb.setVoltage (4.5);
    }

    inline float processSample (float inSamp) noexcept
    {
        Vres.setVoltage (inSamp);
        R.compute();

        return wdft::voltage<float> (C4);
    }

    void setParams (float distParam)
    {
        ResDist_R3.setResistanceValue (distParam * rDistVal + R3Val);
    }
};

class MXRDistPlus2
{
private:
    ResVs Vres { 10.0e3f }; //encompasses R5 = 10k
    Res ResOut { 10.0e3f }; //output potentiometer

    Cap C5 { 1.0e-9f };

    wdft::WDFParallelT<float, Res, Cap> P3 { ResOut, C5 };
    wdft::WDFParallelT<float, decltype (P3), ResVs> P4 { P3, Vres };

    wdft::DiodePairT<float, decltype (P4), wdft::DiodeQuality::Best> DP { P4, 2.52e-9f, 25.85e-3f * 1.75f };

public:
    MXRDistPlus2() = default;

    void prepare (float sampleRate)
    {
        C5.prepare (sampleRate);
    }

    inline float processSample (float inSamp) noexcept
    {
        Vres.setVoltage (inSamp);

        DP.incident (P4.reflected());
        P4.incident (DP.reflected());

        return wdft::voltage<float> (ResOut);
    }
};
} // namespace MXRDistWDFs

class MXRDistWDF
{
public:
    MXRDistWDF() = default;

    void prepare (double sampleRate)
    {
        wdf1.prepare ((float) sampleRate);
        wdf2.prepare ((float) sampleRate);
    }

    void setParameters (float distParam)
    {
        wdf1.setParams (distParam);
    }

    inline float processSample (float x) noexcept
    {
        auto v1 = wdf1.processSample (x);
        return wdf2.processSample (v1);
    }

    void process (float* buffer, const int numSamples)
    {
        for (int n = 0; n < numSamples; ++n)
            buffer[n] = processSample (buffer[n]);
    }

private:
    MXRDistWDFs::MXRDistPlus1 wdf1;
    MXRDistWDFs::MXRDistPlus2 wdf2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MXRDistWDF)
};
