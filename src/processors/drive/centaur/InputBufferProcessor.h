#pragma once
#include <pch.h>

class InputBufferProcessor : public chowdsp::IIRFilter<1>
{
public:
    InputBufferProcessor() = default;

    void prepare (float sample_rate, int num_channels)
    {
        chowdsp::IIRFilter<1>::prepare (num_channels);
        fs = sample_rate;

        calc_coefs();
    }

    void calc_coefs()
    {
        // analog coefficients
        float as[2], bs[2];
        bs[0] = C1 * R2;
        bs[1] = 0.0f;
        as[0] = C1 * (R1 + R2);
        as[1] = 1.0f;

        // bilinear transform
        const auto K = 2.0f * fs;
        chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b, a, bs, as, K);
    }

    float R1 = 10.0e3f;
    float R2 = 1.0e6f;
    float C1 = 0.1e-6f;

private:
    float fs = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputBufferProcessor)
};
