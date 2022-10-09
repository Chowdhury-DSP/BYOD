#pragma once

#include <pch.h>

#define BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE(stage, prevElem, Cval)                                        \
    wdft::CapacitorT<float> C##stage { Cval };                                                           \
    wdft::WDFParallelT<float, decltype (prevElem), decltype (C##stage)> P##stage { prevElem, C##stage }; \
    wdft::InductorT<float> L##stage { 500.0e-3f };                                                       \
    wdft::WDFSeriesT<float, decltype (L##stage), decltype (P##stage)> S##stage { L##stage, P##stage }

#define BYOD_SCANNER_VIBRATO_MAKE_RLC_STAGE(stage, prevElem, Cval, RmVal, RpVal)                                    \
    wdft::CapacitorT<float> C##stage { Cval };                                                                      \
    wdft::WDFParallelT<float, decltype (prevElem), decltype (C##stage)> P##stage { prevElem, C##stage };            \
    wdft::InductorT<float> L##stage { 500.0e-3f };                                                                  \
    wdft::WDFSeriesT<float, decltype (L##stage), decltype (P##stage)> S##stage { L##stage, P##stage };              \
    wdft::ResistorT<float> R##stage##p { RpVal };                                                                   \
    wdft::ResistorT<float> R##stage##m { RmVal };                                                                   \
    wdft::WDFSeriesT<float, decltype (R##stage##p), decltype (R##stage##m)> SR##stage { R##stage##p, R##stage##m }; \
    wdft::WDFSeriesT<float, decltype (SR##stage), decltype (S##stage)> PR##stage { SR##stage, S##stage }

// Reference: http://dafx16.vutbr.cz/dafxpapers/38-DAFx-16_paper_54-PN.pdf
class ScannerVibratoWDF
{
public:
    ScannerVibratoWDF() = default;

    static constexpr int numTaps = 9;

    enum class Mode
    {
        V1 = 0,
        V2,
        V3,
        C1,
        C2,
        C3,
    };

    void prepare (float sampleRate)
    {
        static constexpr auto Omega0 = 7075.0f;
        const auto fs_prime = Omega0 / (2.0f * std::tan (Omega0 / (2.0f * sampleRate)));
        C1.prepare (fs_prime);
        C2.prepare (fs_prime);
        C3.prepare (fs_prime);
        C4.prepare (fs_prime);
        C5.prepare (fs_prime);
        C6.prepare (fs_prime);
        C7.prepare (fs_prime);
        C8.prepare (fs_prime);
        C9.prepare (fs_prime);
        C10.prepare (fs_prime);
        C11.prepare (fs_prime);
        C12.prepare (fs_prime);
        C13.prepare (fs_prime);
        C14.prepare (fs_prime);
        C15.prepare (fs_prime);
        C16.prepare (fs_prime);
        C17.prepare (fs_prime);
        C18.prepare (fs_prime);
        L1.prepare (fs_prime);
        L2.prepare (fs_prime);
        L3.prepare (fs_prime);
        L4.prepare (fs_prime);
        L5.prepare (fs_prime);
        L6.prepare (fs_prime);
        L7.prepare (fs_prime);
        L8.prepare (fs_prime);
        L9.prepare (fs_prime);
        L10.prepare (fs_prime);
        L11.prepare (fs_prime);
        L12.prepare (fs_prime);
        L13.prepare (fs_prime);
        L14.prepare (fs_prime);
        L15.prepare (fs_prime);
        L16.prepare (fs_prime);
        L17.prepare (fs_prime);
        L18.prepare (fs_prime);
    }

    template <Mode mode>
    void processBlock (const float* inData, float** outData, int numSamples) noexcept
    {
        if constexpr (mode == Mode::V1 || mode == Mode::V2 || mode == Mode::V3)
            Rc.setResistanceValue (0.0f);
        else if constexpr (mode == Mode::C1 || mode == Mode::C2 || mode == Mode::C3)
            Rc.setResistanceValue (RcVal);

        float* out1 = outData[0];
        float* out2 = outData[1];
        float* out3 = outData[2];
        float* out4 = outData[3];
        float* out5 = outData[4];
        float* out6 = outData[5];
        float* out7 = outData[6];
        float* out8 = outData[7];
        float* out9 = outData[8];

        for (int n = 0; n < numSamples; ++n)
        {
            Vin.setVoltage (inData[n]);

            Vin.incident (Sc.reflected());
            Sc.incident (Vin.reflected());

            const auto VRc = wdft::voltage<float> (Rc);

            out1[n] = VRc + wdft::voltage<float> (R1m);
            out2[n] = VRc + wdft::voltage<float> (R2m);
            if constexpr (mode == Mode::V1 || mode == Mode::C1)
            {
                out3[n] = VRc + wdft::voltage<float> (R3m); // V3
                out4[n] = VRc + wdft::voltage<float> (R4m); // V4
                out5[n] = VRc + wdft::voltage<float> (R5m); // V5
                out6[n] = VRc + wdft::voltage<float> (R6m); // V6
                out7[n] = VRc + wdft::voltage<float> (C6); // V7
                out8[n] = VRc + wdft::voltage<float> (C7); // V8
                out9[n] = VRc + wdft::voltage<float> (C8); // V9
            }
            else if constexpr (mode == Mode::V2 || mode == Mode::C2)
            {
                out3[n] = VRc + wdft::voltage<float> (R3m); // V3
                out4[n] = VRc + wdft::voltage<float> (R5m); // V5
                out5[n] = VRc + wdft::voltage<float> (C6); // V7
                out6[n] = VRc + wdft::voltage<float> (C8); // V9
                out7[n] = VRc + wdft::voltage<float> (C10); // V11
                out8[n] = VRc + wdft::voltage<float> (C11); // V12
                out9[n] = VRc + wdft::voltage<float> (C12); // V13
            }
            else if constexpr (mode == Mode::V3 || mode == Mode::C3)
            {
                out3[n] = VRc + wdft::voltage<float> (R4m); // V4
                out4[n] = VRc + wdft::voltage<float> (C6); // V7
                out5[n] = VRc + wdft::voltage<float> (C9); // V10
                out6[n] = VRc + wdft::voltage<float> (C12); // V13
                out7[n] = VRc + wdft::voltage<float> (C15); // V16
                out8[n] = VRc + wdft::voltage<float> (C17); // V18
                out9[n] = VRc + wdft::voltage<float> (C18); // V19
            }
        }
    }

private:
    wdft::ResistorT<float> Rt { 15.0e3f };
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (18, Rt, 0.001e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (17, S18, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (16, S17, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (15, S16, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (14, S15, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (13, S14, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (12, S13, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (11, S12, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (10, S11, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (9, S10, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (8, S9, 0.004e-6f);
    BYOD_SCANNER_VIBRATO_MAKE_LC_STAGE (7, S8, 0.004e-6f);

    BYOD_SCANNER_VIBRATO_MAKE_RLC_STAGE (6, S7, 0.004e-6f, 0.18e6f, 12.0e3f);
    BYOD_SCANNER_VIBRATO_MAKE_RLC_STAGE (5, PR6, 0.004e-6f, 0.18e6f, 18.0e3f);
    BYOD_SCANNER_VIBRATO_MAKE_RLC_STAGE (4, PR5, 0.004e-6f, 0.18e6f, 33.0e3f);
    BYOD_SCANNER_VIBRATO_MAKE_RLC_STAGE (3, PR4, 0.004e-6f, 0.15e6f, 39.0e3f);
    BYOD_SCANNER_VIBRATO_MAKE_RLC_STAGE (2, PR3, 0.004e-6f, 0.15e6f, 56.0e3f);
    BYOD_SCANNER_VIBRATO_MAKE_RLC_STAGE (1, PR2, 0.004e-6f, 68.0e3f, 27.0e3f);

    static constexpr auto RcVal = 22.0e3f;
    wdft::ResistorT<float> Rc { RcVal };
    wdft::WDFSeriesT<float, decltype (Rc), decltype (PR1)> Sc { Rc, PR1 };

    wdft::IdealVoltageSourceT<float, decltype (Sc)> Vin { Sc };
};
