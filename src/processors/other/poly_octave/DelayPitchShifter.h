#pragma once

#include <pch.h>

namespace pitch_shift
{
template <typename T>
struct Delay
{
    std::vector<T> delay_buffer {};
    size_t write_pointer = 0;
    T read_pointer {};
    size_t N_int = 0;
    T N {};
    T phase_offset_01 {};
    T read_pointer_increment = (T) 1;

    void prepare (double sample_rate, T initial_phase_offset_01 = {})
    {
        N_int = static_cast<size_t> (sample_rate * 0.06);
        delay_buffer.resize (2 * N_int, T {});
        N = static_cast<T> (N_int);
        write_pointer = 0;

        phase_offset_01 = initial_phase_offset_01;
        read_pointer = N * phase_offset_01;
    }

    void reset()
    {
        std::fill (delay_buffer.begin(), delay_buffer.end(), T {});
        write_pointer = 0;
        read_pointer = N * phase_offset_01;
    }

    T get_phase_01() const noexcept
    {
        const auto offset = static_cast<T> (write_pointer);
        const auto wrapped_offset = read_pointer >= offset ? offset : offset - N;
        return (read_pointer - wrapped_offset) / N;
    }

    T process_sample (T x) noexcept
    {
        delay_buffer[write_pointer] = x;
        delay_buffer[write_pointer + N_int] = x;

        const auto rp_int = static_cast<size_t> (read_pointer);
        const auto alpha = read_pointer - static_cast<T> (rp_int);
        const auto y = ((T) 1 - alpha) * delay_buffer[rp_int] + alpha * delay_buffer[rp_int + 1];

        read_pointer = std::fmod (read_pointer + read_pointer_increment, N);
        write_pointer = (write_pointer + 1) % N_int;

        const auto amplitude_mod = (T) 0.5 * ((T) 1 - math_approx::cos<5> (juce::MathConstants<T>::twoPi * get_phase_01()));
        return y * amplitude_mod;
    }
};

template <typename T>
struct Processor
{
    Delay<T> d_0 {};
    Delay<T> d_half {};

    void prepare (double sample_rate)
    {
        d_0.prepare (sample_rate);
        d_half.prepare (sample_rate, (T) 0.5);
    }

    void reset()
    {
        d_0.reset();
        d_half.reset();
    }

    void set_pitch_factor (T shift_factor)
    {
        d_0.read_pointer_increment = shift_factor;
        d_half.read_pointer_increment = shift_factor;
    }

    T process_sample (T x) noexcept
    {
        return d_0.process_sample (x) + d_half.process_sample (x);
    }
};
} // namespace pitch_shift
