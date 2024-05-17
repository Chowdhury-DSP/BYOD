#pragma once

#include "PolyOctaveV2FilterBank.h"

namespace poly_octave_v2
{
// Reference for filter-bank design and octave shifting:
// https://aaltodoc.aalto.fi/server/api/core/bitstreams/ff9e52cf-fd79-45eb-b695-93038244ec0e/content

template <size_t N>
void design_filter_bank (std::array<ComplexERBFilterBank<N>, 2>& filter_bank,
                         double gamma,
                         double erb_start,
                         double q_ERB,
                         double sample_rate);

template <size_t num_octaves_up, size_t N>
void process (ComplexERBFilterBank<N>& filter_bank,
              const float* buffer_in,
              float* buffer_out,
              int num_samples) noexcept;

template <size_t num_octaves_up, size_t N>
void process_avx (ComplexERBFilterBank<N>& filter_bank,
                  const float* buffer_in,
                  float* buffer_out,
                  int num_samples) noexcept;
} // namespace poly_octave_v2
