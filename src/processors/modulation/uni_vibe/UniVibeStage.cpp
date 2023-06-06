#include "UniVibeStage.h"

void UniVibeStage::prepare (double sampleRate, int samplesPerBlock)
{
    ldrLFOData.resize ((size_t) samplesPerBlock);

    T = 1.0f / (float) sampleRate;
    H_c.prepare (2);
    H_e.prepare (2);
}

void UniVibeStage::reset()
{
    H_c.reset();
    H_e.reset();
}

static void fillLDRData (const float* modData, const float* intensityData, float* ldrData, int numSamples, const UniVibeStage::LDRMap& map)
{
    for (int n = 0; n < numSamples; ++n)
        ldrData[n] = map.B * chowdsp::PowApprox::exp (map.C * modData[n] * intensityData[n]) + map.A;
}

void UniVibeStage::process (const AudioBuffer<float>& bufferIn,
                            AudioBuffer<float>& bufferOut,
                            const float* modData,
                            const float* intensityData) noexcept
{
    const auto numChannels = bufferIn.getNumChannels();
    const auto numSamples = bufferIn.getNumSamples();
    fillLDRData (modData, intensityData, ldrLFOData.data(), numSamples, ldrMap);

    const auto kappa_c = C_p / (C_dc + C_p);
    const auto kappa_e = C_dc / (C_dc + C_p);
    const auto omega_c = (C_dc + C_p) / (C_dc * C_p);

    const auto double_bilinear = [] (float (&b1)[2], float (&b2)[2], float (&a)[2], const float (&bs1)[2], const float (&bs2)[2], const float (&as)[2], float K)
    {
        const auto a0_inv = 1.0f / (as[0] * K + as[1]);
        b1[0] = (bs1[0] * K + bs1[1]) * a0_inv;
        b1[1] = (bs1[0] * -K + bs1[1]) * a0_inv;
        b2[0] = (bs2[0] * K + bs2[1]) * a0_inv;
        b2[1] = (bs2[0] * -K + bs2[1]) * a0_inv;
        a[0] = 1.0f;
        a[1] = (as[0] * -K + as[1]) * a0_inv;
    };

    using chowdsp::Math::algebraicSigmoid;
    static constexpr auto driveGain = 3.0f;
    static constexpr auto driveGainRecip = 1.0f / driveGain;
    static constexpr auto driveBias = 0.33f;
    static const auto driveBiasInv = algebraicSigmoid (driveBias);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* xData = bufferIn.getReadPointer (ch);
        auto* yData = bufferOut.getWritePointer (ch);
        for (int n = 0; n < numSamples; ++n)
        {
            const auto omega_0 = omega_c / (R6 + ldrLFOData[(size_t) n]);
            const auto K = omega_0 / juce::dsp::FastMathApproximations::tan (omega_0 * 0.5f * T);

            const float b_cs[] = { 1.0f, kappa_c * omega_0 };
            const float b_es[] = { 0.0f, kappa_e * omega_0 };
            const float a_s[] = { 1.0f, omega_0 };
            float b_cz[2];
            float b_ez[2];
            float a_z[2];
            double_bilinear (b_cz, b_ez, a_z, b_cs, b_es, a_s, K);
            H_c.setCoefs (b_cz, a_z);
            H_e.setCoefs (b_ez, a_z);

            const auto x_drive_p = (algebraicSigmoid (driveGain * xData[n] + driveBias) - driveBiasInv) * driveGainRecip;
            const auto x_drive_m = (algebraicSigmoid (driveGain * xData[n] - driveBias) + driveBiasInv) * driveGainRecip;

            yData[n] = alpha * H_e.processSample (x_drive_m, ch) - beta * H_c.processSample (x_drive_p, ch);
        }
    }
}
