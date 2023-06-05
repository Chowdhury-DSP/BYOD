#pragma once

#include <pch.h>

namespace Phase90Filters
{
namespace detail
{
    inline float fastSigmoid (float x)
    {
        return 2.0f * chowdsp::Math::algebraicSigmoid (0.5f * x);
    }
} // namespace detail

template <bool withFeedback = false>
struct Phase90_1Stage : chowdsp::IIRFilter<1>
{
    void prepare (float sampleRate)
    {
        fs = sampleRate;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
        reset();
    }

    void setResistor (float newR6)
    {
        R6 = newR6;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
    }

    void setCapacitor (float newC1)
    {
        C1 = newC1;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
    }

    void processStage (const float* input,
                       float* output,
                       const float* modulation01,
                       [[maybe_unused]] const float* feedback,
                       int numSamples) noexcept
    {
        chowdsp::ScopedValue z1 { z[0][1] };
        for (int n = 0; n < numSamples; ++n)
        {
            const auto k = C1 * R6 * modulation01[n];
            chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b, a, { k, -1.0f }, { k, 1.0f }, K);

            if constexpr (withFeedback)
                chowdsp::LinearTransforms::transformFeedback<1> (b, a, feedback[n]);

            output[n] = processSample1stOrder (input[n], z1.get());
            output[n] = detail::fastSigmoid (output[n]);
        }
    }

private:
    float fs = 48000.0f;
    float K = 1.0f;
    float C1 = 47.0e-9f;
    float R6 = 24.0e3f;
};

template <bool withFeedback = false>
struct Phase90_2Stage : chowdsp::IIRFilter<2>
{
    void prepare (float sampleRate)
    {
        fs = sampleRate;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
        reset();
    }

    void setResistor (float newR6)
    {
        R6 = newR6;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
    }

    void setCapacitor (float newC1)
    {
        C1 = newC1;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
    }

    void processStage (const float* input,
                       float* output,
                       const float* modulation01,
                       [[maybe_unused]] const float* feedback,
                       int numSamples) noexcept
    {
        chowdsp::ScopedValue z1 { z[0][1] };
        chowdsp::ScopedValue z2 { z[0][2] };
        for (int n = 0; n < numSamples; ++n)
        {
            const auto k = C1 * R6 * modulation01[n];
            chowdsp::ConformalMaps::Transform<float, 2>::bilinear (b, a, { k * k, -2 * k, 1.0f }, { k * k, 2 * k, 1.0f }, K);

            if constexpr (withFeedback)
                chowdsp::LinearTransforms::transformFeedback<2> (b, a, feedback[n]);

            output[n] = processSample2ndOrder (input[n], z1.get(), z2.get());
            output[n] = detail::fastSigmoid (output[n]);
        }
    }

private:
    float fs = 48000.0f;
    float K = 1.0f;
    float C1 = 47.0e-9f;
    float R6 = 24.0e3f;
};

template <bool withFeedback = false>
struct Phase90_3Stage : chowdsp::IIRFilter<3>
{
    void prepare (float sampleRate)
    {
        fs = sampleRate;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
        reset();
    }

    void setResistor (float newR6)
    {
        R6 = newR6;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
    }

    void setCapacitor (float newC1)
    {
        C1 = newC1;
        K = chowdsp::ConformalMaps::computeKValueAngular (1.0f / (C1 * R6), fs);
    }


    void processStage (const float* input,
                       float* output,
                       const float* modulation01,
                       [[maybe_unused]] const float* feedback,
                       int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
        {
            const auto k = C1 * R6 * modulation01[n];
            chowdsp::ConformalMaps::Transform<float, 3>::bilinear (b,
                                                                   a,
                                                                   { k * k * k, -3 * k * k, 3 * k, -1.0f },
                                                                   { k * k * k, +3 * k * k, 3 * k, +1.0f },
                                                                   K);

            if constexpr (withFeedback)
                chowdsp::LinearTransforms::transformFeedback<3> (b, a, feedback[n]);

            output[n] = processSample (input[n]);
            output[n] = detail::fastSigmoid (output[n]);
        }
    }

private:
    float fs = 48000.0f;
    float K = 1.0f;
    float C1 = 47.0e-9f;
    float R6 = 24.0e3f;
};

struct Phase90_FB2
{
    void prepare (float sampleRate)
    {
        stage1.prepare (sampleRate);
        stage2_4.prepare (sampleRate);
    }

    void setResistor (float newR6)
    {
        stage1.setResistor (newR6);
        stage2_4.setResistor (newR6);
    }

    void setCapacitor (float newC1)
    {
        stage1.setCapacitor (newC1);
        stage2_4.setCapacitor (newC1);
    }

    void processBlock (const float* input, float* output, const float* modulation01, const float* feedback, int numSamples) noexcept
    {
        stage1.processStage (input, output, modulation01, feedback, numSamples);
        stage2_4.processStage (output, output, modulation01, feedback, numSamples);
    }

private:
    Phase90_1Stage<> stage1;
    Phase90_3Stage<true> stage2_4;
};

struct Phase90_FB3
{
    void prepare (float sampleRate)
    {
        stage1_2.prepare (sampleRate);
        stage3_4.prepare (sampleRate);
    }

    void setResistor (float newR6)
    {
        stage1_2.setResistor (newR6);
        stage3_4.setResistor (newR6);
    }

    void setCapacitor (float newC1)
    {
        stage1_2.setCapacitor (newC1);
        stage3_4.setCapacitor (newC1);
    }

    void processBlock (const float* input, float* output, const float* modulation01, const float* feedback, int numSamples) noexcept
    {
        stage1_2.processStage (input, output, modulation01, feedback, numSamples);
        stage3_4.processStage (output, output, modulation01, feedback, numSamples);
    }

private:
    Phase90_2Stage<> stage1_2;
    Phase90_2Stage<true> stage3_4;
};

struct Phase90_FB4
{
    void prepare (float sampleRate)
    {
        stage1_3.prepare (sampleRate);
        stage4.prepare (sampleRate);
    }

    void setResistor (float newR6)
    {
        stage1_3.setResistor (newR6);
        stage4.setResistor (newR6);
    }

    void setCapacitor (float newC1)
    {
        stage1_3.setCapacitor (newC1);
        stage4.setCapacitor (newC1);
    }

    void processBlock (const float* input, float* output, const float* modulation01, const float* feedback, int numSamples) noexcept
    {
        stage1_3.processStage (input, output, modulation01, feedback, numSamples);
        stage4.processStage (output, output, modulation01, feedback, numSamples);
    }

private:
    Phase90_3Stage<> stage1_3;
    Phase90_1Stage<true> stage4;
};
} // namespace Phase90Filters