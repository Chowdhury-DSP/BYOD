#pragma once

#include <pch.h>

using namespace chowdsp::WDF;

class BassmanToneStack
{
public:
    BassmanToneStack() = default;

    void setSMatrixData();
    void setParams (float pot1, float pot2, float pot3, bool force = false);

    void prepare (double sr);
    void process (float* buffer, const int numSamples) noexcept;

private:
    inline float processSample (float inSamp) noexcept
    {
        Vres.setVoltage (inSamp);
        R.incident (0.0f);

        return wdft::voltage<float> (Res1m) + wdft::voltage<float> (S2) + wdft::voltage<float> (Res3m);
    }

    using Res = wdft::ResistorT<float>;
    using ResVs = wdft::ResistiveVoltageSourceT<float>;
    using Cap = wdft::CapacitorT<float>;

    Cap Cap1 { 250e-12f };
    Cap Cap2 { 20e-9f }; // Port D
    Cap Cap3 { 20e-9f }; // Port F

    Res Res1p { 1.0f };
    Res Res1m { 1.0f };
    Res Res2 { 1.0f };
    Res Res3p { 1.0f };
    Res Res3m { 1.0f };
    Res Res4 { 56e3f }; // Port E

    ResVs Vres { 1.0f };

    // Port A
    wdft::WDFSeriesT<float, decltype (Vres), decltype (Res3m)> S1 { Vres, Res3m };

    // Port B
    wdft::WDFSeriesT<float, decltype (Res2), decltype (Res3p)> S3 { Res2, Res3p };

    // Port C
    wdft::WDFSeriesT<float, decltype (Res1p), decltype (Res1m)> S4 { Res1p, Res1m };
    wdft::WDFSeriesT<float, decltype (Cap1), decltype (S4)> S2 { Cap1, S4 };

    static constexpr auto R1 = 250e3f;
    static constexpr auto R2 = 1e6f;
    static constexpr auto R3 = 96e3f; // modified from 25e3

    wdft::RootRtypeAdaptor<float, decltype (S1), decltype (S3), decltype (S2), Cap, Res, Cap> R { std::tie (S1, S3, S2, Cap2, Res4, Cap3) };

    SmoothedValue<float, ValueSmoothingTypes::Linear> pot1Smooth, pot2Smooth, pot3Smooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassmanToneStack)
};
