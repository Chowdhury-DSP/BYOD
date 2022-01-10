#pragma once

#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(x)

#include "../../modules/chowdsp_utils/modules/chowdsp_dsp/Filters/chowdsp_IIRFilter.h"
#include <wdf_t.h>

namespace wdft = chowdsp::WDFT;

class BigMuffClip
{
public:
    BigMuffClip() = default;

    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;

        G_C_12 = (2.0f * 470.0e-12f * fs);

        constexpr float C5 = 100.0e-9f;
        constexpr float R19 = 10.0e3f;
        constexpr float R20 = 100.0e3f;

        float b_s[] = { C5 * R20, 0.0f };
        float a_s[] = { C5 * (R19 + R20), 1.0f };
        float b[2];
        float a[2];
        bilinear2 (b, a, b_s, a_s, 2.0f * fs);
        inputFilter.setCoefs (b, a);

        reset();
    }

    static inline void bilinear2 (float (&b)[2], float (&a)[2], const float (&bs)[2], const float (&as)[2], float K)
    {
        const auto a0 = as[0] * K + as[1];
        b[0] = (bs[0] * K + bs[1]) / a0;
        b[1] = (-bs[0] * K + bs[1]) / a0;
        a[0] = 1.0f;
        a[1] = (-as[0] * K + as[1]) / a0;
    }

    void reset()
    {
        inputFilter.reset();

        y_1 = 0.0f;
        C_12_1 = 0.0f;
    }

    inline float process (float x) noexcept
    {
        auto u_n = inputFilter.processSample (x);

        constexpr float Vt = 25.85e-3f;
        constexpr float twoIs = 2.0f * 2.52e-9f;
        constexpr float R20 = 100.0e3f;
        constexpr float G17 = 1.0f / 470.0e3f;
        constexpr float A = -10000.0f / 150.0f;

        // newton-raphson...
        float y_k = y_1;
        for (int n = 0; n < 5; ++n)
        {
            auto v_drop = y_k - 0.7f;

            auto i_diodes = twoIs * std::sinh (v_drop / Vt);
            auto di_diodes = (twoIs / Vt) * std::cosh (v_drop / Vt);

            auto i_R17 = v_drop * G17;
            auto i_C12 = v_drop * G_C_12 - C_12_1;

            auto F_y = A * (u_n + R20 * (i_diodes + i_R17 + i_C12)) - y_k;
            auto dF_y = A * (R20 * (di_diodes + G17 + G_C_12)) - 1.0f;
            y_k -= F_y / dF_y;
        }

        // update state
        C_12_1 = 2.0f * (y_k - 0.7f) * G_C_12 - C_12_1;
        y_1 = y_k;

        return y_1;
    }

    float fs = 48000.0f;
    float y_1 = 0.0f;

    float G_C_12 = 1.0f;
    float C_12_1 = 0.0f;

    chowdsp::IIRFilter<1, float> inputFilter;
};
