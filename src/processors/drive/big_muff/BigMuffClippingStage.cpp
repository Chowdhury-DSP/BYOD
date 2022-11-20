#include "BigMuffClippingStage.h"

namespace
{
// circuit component values
constexpr float C5 = 100.0e-9f;
constexpr float R19 = 10.0e3f;
constexpr float R20 = 100.0e3f;
// from http://www.kitrae.net/music/big_muff_guts.html
constexpr float C12 = 500.0e-12f;
constexpr float G17 = 1.0f / 470.0e3f;

// diode constants
constexpr float Vt = 25.85e-3f;
constexpr float twoIs = 2.0f * 2.52e-9f;
constexpr float twoIs_over_Vt = twoIs / Vt;

constexpr float A = -10000.0f / 150.0f; // BJT Common-Emitter amp gain
constexpr float VbiasA = 0.7f; // bias point after input filter

// compute sinh and cosh at the same time so it's faster...
inline auto sinh_cosh (float x) noexcept
{
    // ref: https://en.wikipedia.org/wiki/Hyperbolic_functions#Definitions
    // sinh = (e^(2x) - 1) / (2e^x), cosh = (e^(2x) + 1) / (2e^x)
    // let B = e^x, then sinh = (B^2 - 1) / (2B), cosh = (B^2 + 1) / (2B)
    // simplifying, we get: sinh = 0.5 (B - 1/B), cosh = 0.5 (B + 1/B)

    auto B = std::exp (x);
    auto Br = 0.5f / B;
    B *= 0.5f;

    auto sinh = B - Br;
    auto cosh = B + Br;

    return std::make_pair (sinh, cosh);
}

template <int numIters>
inline float newton_raphson (float x, float y, float C_12_state, float G_C_12) noexcept
{
    for (int k = 0; k < numIters; ++k)
    {
        auto v_drop = y - VbiasA;

        auto [sinh_v, cosh_v] = sinh_cosh (v_drop / Vt);
        auto i_diodes = twoIs * sinh_v;
        auto di_diodes = twoIs_over_Vt * cosh_v;

        auto i_R17 = v_drop * G17;
        auto i_C12 = v_drop * G_C_12 - C_12_state;

        auto F_y = A * (x + R20 * (i_diodes + i_R17 + i_C12)) - y;
        auto dF_y = A * (R20 * (di_diodes + G17 + G_C_12)) - 1.0f;
        y -= F_y / dF_y;
    }

    return y;
}
} // namespace

void BigMuffClippingStage::prepare (double sampleRate)
{
    fs = (float) sampleRate;

    // set coefficients for input filter
    float b_s[] = { C5 * R20, 0.0f };
    float a_s[] = { C5 * (R19 + R20), 1.0f };
    float b[2];
    float a[2];
    chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b, a, b_s, a_s, 2.0f * fs);
    inputFilter[0].setCoefs (b, a);
    inputFilter[1].setCoefs (b, a);

    reset();
}

void BigMuffClippingStage::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        inputFilter[ch].reset();

        y_1[ch] = 0.0f;
        C_12_1[ch] = 0.0f;
    }
}

template <bool highQuality>
void BigMuffClippingStage::processBlock (AudioBuffer<float>& buffer, const float smoothing) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    // capacitor C12 admittance, smoothing adds or removes 200 pF
    float G_C_12 = (2.0f * (C12 + (smoothing - 50.0f) * 4.0e-12f) * fs);


    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        for (int n = 0; n < numSamples; ++n)
        {
            // input filter
            auto u_n = inputFilter[ch].processSample (x[n]);

            // newton-raphson
            float y_k = newton_raphson<(highQuality ? 8 : 4)> (u_n, y_1[ch], C_12_1[ch], G_C_12);

            // update state
            C_12_1[ch] = 2.0f * (y_k - VbiasA) * G_C_12 - C_12_1[ch];
            y_1[ch] = y_k;

            x[n] = y_k;
        }
    }
}

template void BigMuffClippingStage::processBlock<true> (AudioBuffer<float>& buffer, const float smoothing) noexcept;
template void BigMuffClippingStage::processBlock<false> (AudioBuffer<float>& buffer, const float smoothing) noexcept;
