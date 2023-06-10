#pragma once

#include <pch.h>

class OutputStageProcessor : public chowdsp::IIRFilter<1>
{
public:
    OutputStageProcessor() = default;

    void prepare (float sample_rate, int samples_per_block, int num_channels)
    {
        chowdsp::IIRFilter<1>::prepare (num_channels);

        level_smooth.mappingFunction = [] (float level)
        { return juce::jlimit (1.0e-5f, 1.0f, level); };
        level_smooth.setRampLength (0.05);
        level_smooth.prepare ((double) sample_rate, samples_per_block);
    }

    void calc_coefs (float cur_level)
    {
        const float R1 = 560.0f + (1.0f - cur_level) * 10000.0f;
        const float R2 = cur_level * 10000.0f + 1.0f;
        static constexpr auto C1 = 4.7e-6f;

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

    void process_block (const chowdsp::BufferView<float>& buffer, float level_param) noexcept
    {
        level_smooth.process (level_param, buffer.getNumSamples());
        if (level_smooth.isSmoothing())
        {
            const auto level_smooth_data = level_smooth.getSmoothedBuffer();
            for (auto [ch, channel_data] : chowdsp::buffer_iters::channels (buffer))
            {
                for (auto [n, sample] : chowdsp::enumerate (channel_data))
                {
                    calc_coefs (level_smooth_data[n]);
                    sample = processSample (sample, ch);
                }
            }
        }
        else
        {
            calc_coefs (level_smooth.getCurrentValue());
            processBlock (buffer);
        }
    }

private:
    float fs = 44100.0f;
    chowdsp::SmoothedBufferValue<float, juce::ValueSmoothingTypes::Multiplicative> level_smooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputStageProcessor)
};
