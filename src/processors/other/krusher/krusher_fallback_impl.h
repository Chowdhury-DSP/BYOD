#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <span>

struct Krusher_Lofi_Resample_State
{
    double upsample_overshoot = 0.0;
    double downsample_overshoot = 0.0;
};

inline void krusher_init_lofi_resample (Krusher_Lofi_Resample_State* state)
{
    state->upsample_overshoot = 0.0;
    state->downsample_overshoot = 0.0;
}

namespace krusher_detail
{
inline void process_lofi_resample (float** source_buffer,
                                   float** dest_buffer,
                                   int num_channels,
                                   int num_samples_source,
                                   int num_samples_dest,
                                   double resample_factor,
                                   double& overshoot_samples)
{
    // simple S&H lofi resampler
    for (int channel = 0; channel < num_channels; ++channel)
    {
        const auto* source_data = source_buffer[channel];
        auto* dest_data = dest_buffer[channel];

        for (int i = 0; i < num_samples_dest; ++i)
        {
            const auto grab_index = (int) ((double) i * resample_factor + overshoot_samples);
            dest_data[i] = source_data[std::min (grab_index, num_samples_source - 1)];
        }
    }

    overshoot_samples = (double) num_samples_dest * resample_factor - std::floor ((double) num_samples_dest * resample_factor);
}
} // namespace krusher_detail

inline void krusher_process_lofi_downsample ([[maybe_unused]] void* ctx,
                                             Krusher_Lofi_Resample_State* state,
                                             float** buffer,
                                             int num_channels,
                                             int num_samples,
                                             double resample_factor)
{
    const auto ds_buffer_size = (int) std::ceil ((double) num_samples / resample_factor);

    auto* temp_data = (float*) alloca (2 * (size_t) ds_buffer_size * sizeof (float));
    float* ds_buffer[2] = { temp_data, temp_data + ds_buffer_size };

    krusher_detail::process_lofi_resample (buffer, ds_buffer, num_channels, num_samples, ds_buffer_size, resample_factor, state->downsample_overshoot);
    krusher_detail::process_lofi_resample (ds_buffer, buffer, num_channels, ds_buffer_size, num_samples, 1.0 / resample_factor, state->upsample_overshoot);
}

//==============================================
struct Krusher_Bit_Reducer_Filter_State
{
    int32_t p1 {};
    int32_t p2 {};
};

namespace krusher_detail
{
static constexpr uint16_t BIT_MASKS[] = {
    0, // 0
    0x0001, // 1
    0x0003, // 2
    0x0007, // 3
    0x000F, // 4
    0x001F, // 5
    0x003F, // 6
    0x007F, // 7
    0x00FF, // 8
    0x01FF, // 9
    0x03FF, // 10
    0x07FF, // 11
    0x0FFF, // 12
    0x1FFF, // 13
    0x3FFF, // 14
    0x7FFF, // 15
};

struct Bit_Reduction_Block
{
    uint8_t shift_amount {};
    std::array<uint16_t, 16> data {};
};

inline uint16_t encode_sample (uint8_t shift, int bit_depth, int16_t x)
{
    const auto value_unsigned = (uint16_t) (x + (1 << 8));
    return (uint16_t) (value_unsigned >> shift) & BIT_MASKS[bit_depth];
}

inline int16_t decode_sample (uint8_t shift, uint16_t x)
{
    return (int16_t) ((uint16_t) (x << shift) - (1 << 8));
}

inline void bit_reduce_decode (const Bit_Reduction_Block& br_block,
                               std::span<int16_t> out,
                               int filter,
                               Krusher_Bit_Reducer_Filter_State& state)
{
    uint8_t shift_amount = br_block.shift_amount;

    const auto type1_filter = [&state] (int16_t nibble_2r)
    {
        const auto y = int32_t (nibble_2r) + ((state.p1 * 15) >> 4);
        state.p2 = 0;
        state.p1 = y;
        return (int16_t) (y >> 4);
    };

    const auto type2_filter = [&state] (int16_t nibble_2r)
    {
        const auto y = int32_t (nibble_2r) + ((state.p1 * 61) >> 5) - ((state.p2 * 15) >> 4);
        state.p2 = state.p1;
        state.p1 = y;
        return (int16_t) (y >> 5);
    };

    const auto type3_filter = [&state] (int16_t nibble_2r)
    {
        const auto y = int32_t (nibble_2r) + ((state.p1 * 115) >> 6) - ((state.p2 * 13) >> 4);
        state.p2 = state.p1;
        state.p1 = y;
        return (int16_t) (y >> 6);
    };

    for (size_t i = 0; i < 16; ++i)
    {
        if (i >= out.size())
            break;

        const auto brr_sample = br_block.data[i];

        switch (filter)
        {
            case 0:
                out[i] = decode_sample (shift_amount, brr_sample);
                break;

            case 1:
                out[i] = type1_filter (decode_sample (shift_amount, brr_sample));
                break;

            case 2:
                out[i] = type2_filter (decode_sample (shift_amount, brr_sample));
                break;

            case 3:
                out[i] = type3_filter (decode_sample (shift_amount, brr_sample));
                break;

            default:
                break;
        }
    }
}

inline Bit_Reduction_Block bit_reduce_encode (std::span<const int16_t> PCM_data, int bit_depth)
{
    uint8_t shift_best = 0;
    double err_min = std::numeric_limits<double>::max();

    for (uint8_t s = 0; s < uint8_t (16 - bit_depth + 1); ++s)
    {
        auto err_sq_accum = 0.0;
        for (size_t i = 0; i < 16; ++i)
        {
            const auto pred = decode_sample (s, encode_sample (s, bit_depth, PCM_data[i]));
            const auto err = double (PCM_data[i] - pred);
            err_sq_accum += err * err;
        }

        if (err_sq_accum < err_min)
        {
            err_min = err_sq_accum;
            shift_best = s;
        }
    }

    Bit_Reduction_Block brr {};
    brr.shift_amount = shift_best;
    for (size_t i = 0; i < 16; ++i)
        brr.data[i] = encode_sample (shift_best, bit_depth, PCM_data[i]);

    return brr;
}

inline void convert_float_to_int (std::span<const float> dataFloat, std::span<int16_t> dataInt)
{
    for (size_t i = 0; i < dataFloat.size(); ++i)
        dataInt[i] = int16_t (dataFloat[i] * float (1 << 8));
}

inline void convert_int_to_float (std::span<const int16_t> dataInt, std::span<float> dataFloat)
{
    for (size_t i = 0; i < dataFloat.size(); ++i)
        dataFloat[i] = ((float) dataInt[i]) / float (1 << 8);
}
} // namespace krusher_detail

inline void krusher_bit_reduce_process_block (float** buffer,
                                              int32_t num_channels,
                                              int32_t num_samples,
                                              int32_t filter_index,
                                              int32_t bit_depth,
                                              Krusher_Bit_Reducer_Filter_State* filter_states)
{
    static constexpr size_t small_block_size = 16;
    std::array<int16_t, small_block_size> samples_int {};

    for (int channel = 0; channel < num_channels; ++channel)
    {
        auto samples_remaining = num_samples;
        while (samples_remaining > 0)
        {
            const auto samples_to_process = std::min (samples_remaining, (int) small_block_size);

            std::span<float> samples_float_span { buffer[channel] + num_samples - samples_remaining, (size_t) samples_to_process };
            std::fill (samples_int.begin(), samples_int.end(), 0);
            std::span<int16_t> samples_int_span { samples_int.data(), (size_t) samples_to_process };

            krusher_detail::convert_float_to_int (samples_float_span, samples_int_span);

            if (bit_depth < 12)
            {
                const auto br_data = krusher_detail::bit_reduce_encode (samples_int_span, bit_depth);
                krusher_detail::bit_reduce_decode (br_data, samples_int_span, filter_index, filter_states[channel]);
            }

            krusher_detail::convert_int_to_float (samples_int_span, samples_float_span);

            samples_remaining -= samples_to_process;
        }
    }
}
