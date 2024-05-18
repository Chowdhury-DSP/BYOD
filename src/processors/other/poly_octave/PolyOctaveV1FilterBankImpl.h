#pragma once

#include "PolyOctaveV1FilterBank.h"

namespace poly_octave_v1
{
// Reference for filter-bank design and octave shifting:
// https://aaltodoc.aalto.fi/server/api/core/bitstreams/ff9e52cf-fd79-45eb-b695-93038244ec0e/content

static_assert (float_2::size == 2);
static_assert (xsimd::batch<double>::size == 2);
static constexpr auto vec_size = float_2::size;

template <bool isImag = false>
inline float_2 processSample (const chowdsp::IIRFilter<4, float_2>& f, std::array<float_2, 5>& z, float_2 x)
{
    auto y = z[1];
    if constexpr (! isImag)
        y += x * f.b[0]; // for the imaginary filter, we know that b[0] == 0

    z[1] = z[2] + x * f.b[1] - y * f.a[1];
    z[2] = z[3] + x * f.b[2] - y * f.a[2];
    z[3] = z[4] + x * f.b[3] - y * f.a[3];
    z[4] = x * f.b[4] - y * f.a[4];

    return y;
}

static constexpr auto q_c = 4.0;
static auto designERBFilter (size_t erb_index,
                             double gamma,
                             double erb_start,
                             double q_ERB,
                             double sample_rate,
                             double (&b_coeffs_cplx_real)[5],
                             double (&b_coeffs_cplx_imag)[5],
                             double (&a_coeffs_cplx)[5])
{
    const auto q_PS = gamma;

    const auto z = erb_start + static_cast<float> (erb_index) * (q_c / q_ERB);
    const auto center_target_freq = 228.7 * (std::pow (10.0, z / 21.3) - 1.0);
    const auto filter_q = (1.0 / (q_PS * q_ERB)) * (24.7 + 0.108 * center_target_freq);

    double b_coeffs_proto[3];
    double a_coeffs_proto[3];
    chowdsp::CoefficientCalculators::calcSecondOrderBPF (b_coeffs_proto,
                                                         a_coeffs_proto,
                                                         center_target_freq / gamma,
                                                         filter_q * 0.5,
                                                         sample_rate);

    auto pole = (std::sqrt (std::pow (std::complex { a_coeffs_proto[1] }, 2.0) - 4.0 * a_coeffs_proto[2]) - a_coeffs_proto[1]) / 2.0;
    auto conj_pole = std::conj (pole);
    if (std::imag (pole) < 0.0)
        pole = conj_pole;
    else if (std::imag (conj_pole) < 0.0)
        conj_pole = pole;
    auto at = -(pole + conj_pole);
    auto att = pole * conj_pole;

    auto ar1 = std::real (at);
    auto ai1 = std::imag (at);
    auto ar2 = std::real (att);
    auto ai2 = std::imag (att);

    // a[] = 1 + 2 ar1 z + (ai1^2 + ar1^2 + 2 ar2) z^2 + (2 ai1 ai2 + 2 ar1 ar2) z^3 + (ai2^2 + ar2^2) z^4
    a_coeffs_cplx[0] = 1.0;
    a_coeffs_cplx[1] = 2.0 * ar1;
    a_coeffs_cplx[2] = ai1 * ai1 + ar1 * ar1 + 2.0 * ar2;
    a_coeffs_cplx[3] = 2.0 * ai1 * ai2 + 2.0 * ar1 * ar2;
    a_coeffs_cplx[4] = ai2 * ai2 + ar2 * ar2;

    // b_real[] = b0 + (ar1 b0 + b1) z + (ar2 b0 + ar1 b1 + b2) z^2 + (ar2 b1 + ar1 b2) z^3 + ar2 b2 z^4
    b_coeffs_cplx_real[0] = b_coeffs_proto[0];
    b_coeffs_cplx_real[1] = ar1 * b_coeffs_proto[0] + b_coeffs_proto[1];
    b_coeffs_cplx_real[2] = ar2 * b_coeffs_proto[0] + ar1 * b_coeffs_proto[1] + b_coeffs_proto[2];
    b_coeffs_cplx_real[3] = ar2 * b_coeffs_proto[1] + ar1 * b_coeffs_proto[2];
    b_coeffs_cplx_real[4] = ar2 * b_coeffs_proto[2];

    // b_imag[] = -ai1 b0 z - (ai2 b0 + ai1 b1) z^2 - (ai2 b1 + ai1 br) z^3 - ai2 br z^4
    b_coeffs_cplx_imag[0] = 0.0;
    b_coeffs_cplx_imag[1] = -ai1 * b_coeffs_proto[0];
    b_coeffs_cplx_imag[2] = -ai2 * b_coeffs_proto[0] - ai1 * b_coeffs_proto[1];
    b_coeffs_cplx_imag[3] = -ai2 * b_coeffs_proto[1] - ai1 * b_coeffs_proto[2];
    b_coeffs_cplx_imag[4] = -ai2 * b_coeffs_proto[2];

    return center_target_freq;
}

static void designFilterBank (std::array<ComplexERBFilterBank, 2>& filterBank,
                              double gamma,
                              double erb_start,
                              double q_ERB,
                              double sampleRate)
{
    for (size_t kiter = 0; kiter < ComplexERBFilterBank::numFilterBands; kiter += vec_size)
    {
        alignas (16) std::array<float_2::value_type, float_2::size> b_coeffs_cplx_real_simd[5];
        alignas (16) std::array<float_2::value_type, float_2::size> b_coeffs_cplx_imag_simd[5];
        alignas (16) std::array<float_2::value_type, float_2::size> a_coeffs_cplx_simd[5];
        alignas (16) std::array<float_2::value_type, float_2::size> center_freqs;
        for (size_t i = 0; i < float_2::size; ++i)
        {
            const auto k = kiter + i;

            double b_coeffs_cplx_real[5];
            double b_coeffs_cplx_imag[5];
            double a_coeffs_cplx[5];
            center_freqs[i] = designERBFilter (k,
                                               gamma,
                                               erb_start,
                                               q_ERB,
                                               sampleRate,
                                               b_coeffs_cplx_real,
                                               b_coeffs_cplx_imag,
                                               a_coeffs_cplx);

            for (size_t c = 0; c < 5; ++c)
            {
                b_coeffs_cplx_real_simd[c][i] = static_cast<float_2::value_type> (b_coeffs_cplx_real[c]);
                b_coeffs_cplx_imag_simd[c][i] = static_cast<float_2::value_type> (b_coeffs_cplx_imag[c]);
                a_coeffs_cplx_simd[c][i] = static_cast<float_2::value_type> (a_coeffs_cplx[c]);
            }
        }

        for (size_t ch = 0; ch < 2; ++ch)
        {
            filterBank[ch].erbFilterReal[kiter / vec_size].reset();
            filterBank[ch].erbFilterReal[kiter / vec_size].setCoefs ({
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[0].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[1].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[2].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[3].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_real_simd[4].data()),
                                                                     },
                                                                     {
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[0].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[1].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[2].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[3].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[4].data()),
                                                                     });
            filterBank[ch].erbFilterImag[kiter / vec_size].reset();
            filterBank[ch].erbFilterImag[kiter / vec_size].setCoefs ({
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[0].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[1].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[2].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[3].data()),
                                                                         xsimd::load_aligned (b_coeffs_cplx_imag_simd[4].data()),
                                                                     },
                                                                     {
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[0].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[1].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[2].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[3].data()),
                                                                         xsimd::load_aligned (a_coeffs_cplx_simd[4].data()),
                                                                     });
        }
    }
}
} // namespace poly_octave_v1
