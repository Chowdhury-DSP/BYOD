#pragma once

#include <pch.h>

namespace poly_octave_v1
{
using float_2 = xsimd::batch<double>;
struct ComplexERBFilterBank
{
    static constexpr size_t numFilterBands = 44;
    std::array<chowdsp::IIRFilter<4, float_2>, numFilterBands / float_2::size> erbFilterReal, erbFilterImag;
};
} // namespace poly_octave_v1

namespace poly_octave_v2
{
using T = xsimd::batch<float>;
using S = float;
static constexpr auto N1 = 32;
template <size_t N>
struct ComplexERBFilterBank
{
    static constexpr size_t num_filter_bands = N;

    struct ComplexFilter
    {
        std::array<T, 3> b_shared_coeffs {};
        std::array<T, 3> b_real_coeffs {};
        std::array<T, 3> b_imag_coeffs {};
        std::array<T, 3> a_coeffs {};
        std::array<T, 3> z_shared {};
        std::array<T, 3> z_real {};
        std::array<T, 3> z_imag {};
    };

    alignas (32) std::array<S, num_filter_bands> a1 {};
    alignas (32) std::array<S, num_filter_bands> a2 {};

    alignas (32) std::array<S, num_filter_bands> shared_b0 {};
    alignas (32) std::array<S, num_filter_bands> shared_b1 {};
    alignas (32) std::array<S, num_filter_bands> shared_b2 {};

    alignas (32) std::array<S, num_filter_bands> real_b1 {};
    alignas (32) std::array<S, num_filter_bands> real_b2 {};

    alignas (32) std::array<S, num_filter_bands> imag_b1 {};
    alignas (32) std::array<S, num_filter_bands> imag_b2 {};

    alignas (32) std::array<S, num_filter_bands> shared_z1 {};
    alignas (32) std::array<S, num_filter_bands> shared_z2 {};

    alignas (32) std::array<S, num_filter_bands> real_z1 {};
    alignas (32) std::array<S, num_filter_bands> real_z2 {};

    alignas (32) std::array<S, num_filter_bands> imag_z1 {};
    alignas (32) std::array<S, num_filter_bands> imag_z2 {};

    std::array<ComplexFilter, num_filter_bands / T::size> erb_filter_complex;
};
} // namespace poly_octave_v2
