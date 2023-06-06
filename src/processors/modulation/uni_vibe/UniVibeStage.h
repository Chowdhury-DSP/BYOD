#pragma once

#include <pch.h>

struct UniVibeStage
{
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void process (const AudioBuffer<float>& bufferIn,
                  AudioBuffer<float>& bufferOut,
                  const float* modData,
                  const float* intensityData) noexcept;

    /** Map an LFO signal [-1,1] to an LDR resistance value (Ohms) */
    struct LDRMap
    {
        // Reference https://www.desmos.com/calculator/ir12katiwo
        float A = -20.0e3f; // [-22,000, -18,000]
        float B = 325.0e3f; // [300,000, 350,000]
        float C = 2.25f; // [2, 2.5]
    } ldrMap;
    std::vector<float> ldrLFOData;

    float C_dc = 1.0e-6f;
    float C_p = 10.0e-9f;
    float R6 = 4.7e3f;
    float alpha = 1.0f;
    float beta = 1.1f;
    float T = 1.0f / 48000.0f;

    chowdsp::IIRFilter<1> H_c;
    chowdsp::IIRFilter<1> H_e;
};
