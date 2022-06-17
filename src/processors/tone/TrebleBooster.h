#pragma once

#include "../BaseProcessor.h"

class TrebleBooster : public BaseProcessor
{
public:
    explicit TrebleBooster (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    inline void calcCoefs (float curTreble) noexcept
    {
        const float G2 = 1.0f / (1.8e3f + (1.0f - curTreble) * Rpot);
        const float G3 = 1.0f / (4.7e3f + curTreble * Rpot);

        constexpr float wc = G1 / C; // frequency to match
        const auto K = wc / std::tan (wc / (2.0f * fs)); // frequency warp to match transition freq

        // analog coefficients
        float b_s[] { C * (G1 + G2), G1 * (G2 + G3) };
        float a_s[] { C * (G3 - G4), -G4 * (G2 + G3) };

        // bilinear transform
        float aU[2], bU[2];
        chowdsp::ConformalMaps::Transform<float, 1>::bilinear (bU, aU, b_s, a_s, K);

        // flip pole inside unit circle to ensure stability
        float b[] { bU[0] / aU[1], bU[1] / aU[1] };
        float a[] { 1.0f, 1.0f / aU[1] };

        for (auto& filt : iir)
            filt.setCoefs (b, a);
    }

    static constexpr float Rpot = 10e3f;
    static constexpr float C = 3.9e-9f;
    static constexpr float G1 = 1.0f / 100e3f;
    static constexpr float G4 = 1.0f / 100e3f;

    std::atomic<float>* trebleParam = nullptr;

    float fs = 48000.0f;
    SmoothedValue<float, ValueSmoothingTypes::Linear> trebleSmooth;

    chowdsp::IIRFilter<1, float> iir[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrebleBooster)
};
