#pragma once

#include <pch.h>

namespace gain_stage
{
class AmpStage : public chowdsp::IIRFilter<2>
{
public:
    AmpStage()
    {
        r10b_smooth.mappingFunction = [this] (float gain)
        {
            return (1.0f - gain) * 100000.0f + R10;
        };
        r10b_smooth.setRampLength (0.05);
        r10b_smooth.reset (0.0f);
    }

    void prepare (float sample_rate, int samples_per_block, int num_channels)
    {
        chowdsp::IIRFilter<2>::prepare (num_channels);
        fs = sample_rate;

        r10b_smooth.prepare ((double) sample_rate, samples_per_block);
    }

    // component values
    float R10 = 2.0e3f;
    float R11 = 15.0e3f;
    float R12 = 422.0e3f;
    float C7 = 82e-9f;
    float C8 = 390e-12f;

    void calc_coefs (float curR10b) noexcept
    {
        // analog coeffs
        float as[3], bs[3];
        as[0] = C7 * C8 * curR10b * R11 * R12;
        as[1] = C7 * curR10b * R11 + C8 * R12 * (curR10b + R11);
        as[2] = curR10b + R11;
        bs[0] = as[0];
        bs[1] = C7 * R11 * R12 + as[1];
        bs[2] = R12 + as[2];

        // frequency warping
        const float wc = chowdsp::ConformalMaps::calcPoleFreq (as[0], as[1], as[2]);
        const auto K = wc == 0.0f ? 2.0f * fs : wc / std::tan (wc / (2.0f * fs));

        chowdsp::ConformalMaps::Transform<float, 2>::bilinear (b, a, bs, as, K);
    }

    void process_block (const chowdsp::BufferView<float>& buffer, float gain_param) noexcept
    {
        r10b_smooth.process (gain_param, (int) buffer.getNumSamples());
        if (r10b_smooth.isSmoothing())
        {
            const auto r10b_smooth_data = r10b_smooth.getSmoothedBuffer();
            for (auto [ch, channel_data] : chowdsp::buffer_iters::channels (buffer))
            {
                for (auto [n, sample] : chowdsp::enumerate (channel_data))
                {
                    calc_coefs (r10b_smooth_data[n]);
                    sample = processSample (sample, ch);
                }
            }
        }
        else
        {
            calc_coefs (r10b_smooth.getCurrentValue());
            chowdsp::IIRFilter<2>::processBlock (buffer);
        }
    }

private:
    float fs = 44100.0f;
    chowdsp::SmoothedBufferValue<float, juce::ValueSmoothingTypes::Multiplicative> r10b_smooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpStage)
};
} // namespace gain_stage
