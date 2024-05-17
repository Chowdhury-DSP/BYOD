#pragma once

#include <array>

namespace poly_octave_v2
{
static constexpr auto N1 = 32;
template <size_t N>
struct ComplexERBFilterBank
{
    static constexpr size_t num_filter_bands = N;

    alignas (32) std::array<float, num_filter_bands> a1 {};
    alignas (32) std::array<float, num_filter_bands> a2 {};

    alignas (32) std::array<float, num_filter_bands> shared_b0 {};
    alignas (32) std::array<float, num_filter_bands> shared_b1 {};
    alignas (32) std::array<float, num_filter_bands> shared_b2 {};

    alignas (32) std::array<float, num_filter_bands> real_b1 {};
    alignas (32) std::array<float, num_filter_bands> real_b2 {};

    alignas (32) std::array<float, num_filter_bands> imag_b1 {};
    alignas (32) std::array<float, num_filter_bands> imag_b2 {};

    alignas (32) std::array<float, num_filter_bands> shared_z1 {};
    alignas (32) std::array<float, num_filter_bands> shared_z2 {};

    alignas (32) std::array<float, num_filter_bands> real_z1 {};
    alignas (32) std::array<float, num_filter_bands> real_z2 {};

    alignas (32) std::array<float, num_filter_bands> imag_z1 {};
    alignas (32) std::array<float, num_filter_bands> imag_z2 {};
};
} // namespace poly_octave_v2
