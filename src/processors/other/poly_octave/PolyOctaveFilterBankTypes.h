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
