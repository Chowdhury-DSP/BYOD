#include "UniVibeStage.h"

void UniVibeStage::prepare (double sampleRate, int samplesPerBlock)
{
    ldrLFOData.resize ((size_t) samplesPerBlock);

    fs = (float) sampleRate;
    H_c.prepare (2);
    H_e.prepare (2);
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

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* xData = bufferIn.getReadPointer (ch);
        auto* yData = bufferOut.getWritePointer (ch);
        for (int n = 0; n < numSamples; ++n)
        {
            const auto kappa_c = C_p / (C_dc + C_p);
            const auto kappa_e = C_dc / (C_dc + C_p);
            const auto omega_0 = (1.0f / (R6 + ldrLFOData[(size_t) n])) * (C_dc + C_p) / (C_dc * C_p);
            const auto K = chowdsp::ConformalMaps::computeKValueAngular (omega_0, fs);

            const float b_cs[] = { 1.0f, kappa_c * omega_0 };
            const float a_cs[] = { 1.0f, omega_0 };
            float b_cz[2];
            float a_cz[2];
            chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b_cz, a_cz, b_cs, a_cs, K);
            H_c.setCoefs (b_cz, a_cz);

            const float b_es[] = { 0.0f, kappa_e * omega_0 };
            const float a_es[] = { 1.0f, omega_0 };
            float b_ez[2];
            float a_ez[2];
            chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b_ez, a_ez, b_es, a_es, K);
            H_e.setCoefs (b_ez, a_ez);

            yData[n] = alpha * H_e.processSample (xData[n], ch) - beta * H_c.processSample (xData[n], ch);
        }
    }
}
