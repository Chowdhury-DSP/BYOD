#pragma once

#include <pch.h>

namespace gain_stage
{
class SummingAmp : public chowdsp::IIRFilter<1>
{
public:
    SummingAmp() = default;

    void prepare (float sampleRate, int num_channels)
    {
        chowdsp::IIRFilter<1>::prepare (num_channels);
        fs = sampleRate;

        calc_coefs();
    }

    float R20 = 392e3f;
    float C13 = 820e-12f;

    void calc_coefs()
    {
        // analog coefficients
        float as[2], bs[2];
        bs[0] = 0.0f;
        bs[1] = R20;
        as[0] = C13 * R20;
        as[1] = 1.0f;

        const auto K = 2.0f * fs;
        chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b, a, bs, as, K);
    }

private:
    float fs = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SummingAmp)
};
} // namespace gain_stage
