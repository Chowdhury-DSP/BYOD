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
}

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

    std::array<ComplexFilter, num_filter_bands / T::size> erb_filter_complex;
};
}
