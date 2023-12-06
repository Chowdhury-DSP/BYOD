#include "MuffClipperStage.h"

namespace MuffClipperMath
{
// circuit component values
constexpr float C5 = 100.0e-9f;
constexpr float R19 = 10.0e3f;
constexpr float R20 = 100.0e3f;
constexpr float C12 = 470.0e-12f;
constexpr float G17 = 1.0f / 470.0e3f;

// diode constants
constexpr float Vt = 25.85e-3f;
constexpr float twoIs = 2.0f * 2.52e-9f;

constexpr float A = -10000.0f / 150.0f; // BJT Common-Emitter amp gain
constexpr float VbiasA = 0.7f; // bias point after input filter

// compute asymmetric sinh and cosh at the same time so it's faster...
inline auto sinh_cosh_asym (float x, float Vt1_recip, float Vt2_recip) noexcept
{
    // ref: https://en.wikipedia.org/wiki/Hyperbolic_functions#Definitions
    // sinh = (e^(2x) - 1) / (2e^x), cosh = (e^(2x) + 1) / (2e^x)
    // let B = e^x, then sinh = (B^2 - 1) / (2B), cosh = (B^2 + 1) / (2B)
    // simplifying, we get: sinh = 0.5 (B - 1/B), cosh = 0.5 (B + 1/B)

    auto B = 0.5f * std::exp (x * Vt1_recip);
    // asymmetric variant
    auto Br = 0.5f / std::exp (x * Vt2_recip);

    auto sinh = B - Br;
    // cosh is pragmatically defined as the derivative of sinh
    auto cosh = B * Vt1_recip + Br * Vt2_recip;

    return std::make_pair (sinh, cosh);
}

template <int numIters>
inline float newton_raphson (float x, float y, float C_12_state, float G_C_12, float clip1V_recip, float clip2V_recip) noexcept
{
    for (int k = 0; k < numIters; ++k)
    {
        auto v_drop = y - VbiasA;

        auto [sinh_v, cosh_v] = sinh_cosh_asym (v_drop, clip1V_recip, clip2V_recip);
        auto i_diodes = twoIs * sinh_v;
        auto di_diodes = twoIs * cosh_v;

        auto i_R17 = v_drop * G17;
        auto i_C12 = v_drop * G_C_12 - C_12_state;

        auto F_y = A * (x + R20 * (i_diodes + i_R17 + i_C12)) - y;
        auto dF_y = A * (R20 * (di_diodes + G17 + G_C_12)) - 1.0f;
        y -= F_y / dF_y;
    }

    return y;
}
} // namespace MuffClipperMath

void MuffClipperStage::prepare (double sampleRate)
{
    fs = (float) sampleRate;

    // set coefficients for input filter
    float b_s[] = { MuffClipperMath::C5 * MuffClipperMath::R20, 0.0f };
    float a_s[] = { MuffClipperMath::C5 * (MuffClipperMath::R19 + MuffClipperMath::R20), 1.0f };
    float b[2];
    float a[2];
    chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b, a, b_s, a_s, 2.0f * fs);
    inputFilter[0].setCoefs (b, a);
    inputFilter[1].setCoefs (b, a);

    reset();
}

void MuffClipperStage::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        inputFilter[ch].reset();

        y_1[ch] = 0.0f;
        C_12_1[ch] = 0.0f;
    }
}

float MuffClipperStage::getGC12 (float fs, float smoothing)
{
    // capacitor C12 admittance, scaled by smoothing
    return 2.0f * MuffClipperMath::C12 * (smoothing + 1.0f) * fs;
}

float MuffClipperStage::getClipV (float clip)
{
    return (clip + 1.0f) / MuffClipperMath::Vt; // Vt_recip value needed by newton_raphson and sinh_cosh_asym
}

template <bool highQuality>
void MuffClipperStage::processBlock (AudioBuffer<float>& buffer,
                                     const chowdsp::SmoothedBufferValue<float>& clipV1Smoothed,
                                     const chowdsp::SmoothedBufferValue<float>& clipV2Smoothed,
                                     const chowdsp::SmoothedBufferValue<float>& gc12Smoothed) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    const auto processSample = [this] (int ch, float x, float clip1V_recip, float clip2V_recip, float G_C_12)
    {
        // input filter
        auto u_n = inputFilter[ch].processSample (x);

        // newton-raphson
        float y_k = MuffClipperMath::newton_raphson<(highQuality ? 8 : 4)> (u_n, y_1[ch], C_12_1[ch], G_C_12, clip1V_recip, clip2V_recip);

        // update state
        C_12_1[ch] = 2.0f * (y_k - MuffClipperMath::VbiasA) * G_C_12 - C_12_1[ch];
        y_1[ch] = y_k;

        return y_k;
    };

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        if (gc12Smoothed.isSmoothing() || clipV1Smoothed.isSmoothing() || clipV2Smoothed.isSmoothing())
        {
            const auto clipV1_data = clipV1Smoothed.getSmoothedBuffer();
            const auto clipV2_data = clipV2Smoothed.getSmoothedBuffer();
            const auto G_C_12_data = gc12Smoothed.getSmoothedBuffer();
            for (int n = 0; n < numSamples; ++n)
                x[n] = processSample (ch, x[n], clipV1_data[n], clipV2_data[n], G_C_12_data[n]);
        }
        else
        {
            const auto clip1V = clipV1Smoothed.getCurrentValue();
            const auto clip2V = clipV2Smoothed.getCurrentValue();
            const auto G_C_12 = gc12Smoothed.getCurrentValue();
            for (int n = 0; n < numSamples; ++n)
                x[n] = processSample (ch, x[n], clip1V, clip2V, G_C_12);
        }
    }
}

template void MuffClipperStage::processBlock<true> (AudioBuffer<float>&,
                                                    const chowdsp::SmoothedBufferValue<float>&,
                                                    const chowdsp::SmoothedBufferValue<float>&,
                                                    const chowdsp::SmoothedBufferValue<float>&) noexcept;
template void MuffClipperStage::processBlock<false> (AudioBuffer<float>&,
                                                     const chowdsp::SmoothedBufferValue<float>&,
                                                     const chowdsp::SmoothedBufferValue<float>&,
                                                     const chowdsp::SmoothedBufferValue<float>&) noexcept;
