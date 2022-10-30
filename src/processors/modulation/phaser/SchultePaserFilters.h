#pragma once

#include <pch.h>

namespace SchultePhaserFilters
{
static constexpr float Cval = 15.0e-9f;
static constexpr float Rval = 100.0e3f;
static constexpr auto wc = 1.0f / (Cval * Rval);

struct FeedbackStage : chowdsp::IIRFilter<2>
{
    void prepare (float sampleRate)
    {
        K = chowdsp::ConformalMaps::computeKValueAngular (wc, sampleRate);
    }

    void processStage (const float* input, float* output, const float* modulation01, const float* feedback, int numSamples) noexcept
    {
        juce::ignoreUnused (feedback);

        chowdsp::ScopedValue z1 { z[0][1] };
        chowdsp::ScopedValue z2 { z[0][2] };
        for (int n = 0; n < numSamples; ++n)
        {
            const auto k = Cval * Rval * modulation01[n];
            chowdsp::ConformalMaps::Transform<float, 2>::bilinear (b, a, { k * k, -2 * k, 1.0f }, { k * k, 2 * k, 1.0f }, K);
            chowdsp::LinearTransforms::transformFeedback<2> (b, a, feedback[n]);

            output[n] = processSample2ndOrder (input[n], z1.get(), z2.get());
        }
    }

private:
    float K = 1.0f;
};

struct FeedbackStageNoMod : chowdsp::IIRFilter<2>
{
    void prepare (float sampleRate)
    {
        K = chowdsp::ConformalMaps::computeKValueAngular (wc, sampleRate);
    }

    void processStage (const float* input, float* output, const float* modulation01, const float* feedback, int numSamples) noexcept
    {
        juce::ignoreUnused (feedback);

        chowdsp::ScopedValue z1 { z[0][1] };
        chowdsp::ScopedValue z2 { z[0][2] };
        for (int n = 0; n < numSamples; ++n)
        {
            const auto k = Cval * Rval * modulation01[n];
            const auto G = feedback[n];
            chowdsp::ConformalMaps::Transform<float, 2>::bilinear (b,
                                                                   a,
                                                                   { k * k, 2 * k, 1.0f },
                                                                   { k * k * (1.0f + G), 2 * k * (1.0f - G), 1.0f + G },
                                                                   K);

            output[n] = processSample2ndOrder (input[n], z1.get(), z2.get());
        }
    }

private:
    float K = 1.0f;
};

struct ModStages : chowdsp::IIRFilter<2>
{
    void prepare (float sampleRate)
    {
        K = chowdsp::ConformalMaps::computeKValueAngular (wc, sampleRate);
    }

    void processStage (const float* input, float* output, const float* modulation01, const float* feedback, int numSamples) noexcept
    {
        juce::ignoreUnused (feedback);

        chowdsp::ScopedValue z1 { z_custom[0] };
        chowdsp::ScopedValue z2 { z_custom[1] };
        chowdsp::ScopedValue z3 { z_custom[2] };
        chowdsp::ScopedValue z4 { z_custom[3] };
        chowdsp::ScopedValue z5 { z_custom[4] };
        chowdsp::ScopedValue z6 { z_custom[5] };
        for (int n = 0; n < numSamples; ++n)
        {
            const auto k = Cval * Rval * modulation01[n];
            chowdsp::ConformalMaps::Transform<float, 2>::bilinear (b, a, { k * k, -2 * k, 1.0f }, { k * k, 2 * k, 1.0f }, K);

            output[n] = processSample2ndOrder (input[n], z1.get(), z2.get());
            output[n] = processSample2ndOrder (output[n], z3.get(), z4.get());
            output[n] = processSample2ndOrder (output[n], z5.get(), z6.get());
        }
    }

private:
    float z_custom[6];
    float K = 1.0f;
};
} // namespace SchultePhaserFilters
