#include "PolyOctaveV2FilterBankImpl.h"

#include <cmath>
#include <complex>

#if defined(__AVX__)
#include <immintrin.h>
#elif defined(__SSE2__)
#include <immintrin.h>
#elif defined(__ARM_NEON__)
#include <arm_neon.h>
#else
#warning "Compiling poly octave DSP without SIMD optimization"
#endif

namespace poly_octave_v2
{
// Reference for filter-bank design and octave shifting:
// https://aaltodoc.aalto.fi/server/api/core/bitstreams/ff9e52cf-fd79-45eb-b695-93038244ec0e/content

#if defined(__ARM_NEON__)
inline std::pair<float32x4_t, float32x4_t> process_sample_neon (float32x4_t x,
                                                                float32x4_t a1,
                                                                float32x4_t a2,
                                                                float32x4_t shared_b0,
                                                                float32x4_t shared_b1,
                                                                float32x4_t shared_b2,
                                                                float32x4_t real_b1,
                                                                float32x4_t real_b2,
                                                                float32x4_t imag_b1,
                                                                float32x4_t imag_b2,
                                                                float32x4_t& shared_z1,
                                                                float32x4_t& shared_z2,
                                                                float32x4_t& real_z1,
                                                                float32x4_t& real_z2,
                                                                float32x4_t& imag_z1,
                                                                float32x4_t& imag_z2)
{
    const auto y_shared = vfmaq_f32 (shared_z1, x, shared_b0);
    shared_z1 = vfmsq_f32 (vfmaq_f32 (shared_z2, x, shared_b1), y_shared, a1);
    shared_z2 = x * shared_b2 - y_shared * a2;

    const auto y_real = vaddq_f32 (real_z1, y_shared); // for the real filter, we know that b[0] == 1
    real_z1 = vfmsq_f32 (vfmaq_f32 (real_z2, y_shared, real_b1), y_real, a1);
    real_z2 = vfmsq_f32 (vmulq_f32 (y_shared, real_b2), y_real, a2);

    const auto y_imag = imag_z1; // for the imaginary filter, we know that b[0] == 0
    imag_z1 = vfmsq_f32 (vfmaq_f32 (imag_z2, y_shared, imag_b1), y_imag, a1);
    imag_z2 = vfmsq_f32 (vmulq_f32 (y_shared, imag_b2), y_imag, a2);

    return { y_real, y_imag };
}
#elif defined(__AVX__) || defined(__SSE2__)
#if defined (__AVX__)
inline std::pair<__m256, __m256> process_sample_avx (__m256 x,
                                                     __m256 a1,
                                                     __m256 a2,
                                                     __m256 shared_b0,
                                                     __m256 shared_b1,
                                                     __m256 shared_b2,
                                                     __m256 real_b1,
                                                     __m256 real_b2,
                                                     __m256 imag_b1,
                                                     __m256 imag_b2,
                                                     __m256& shared_z1,
                                                     __m256& shared_z2,
                                                     __m256& real_z1,
                                                     __m256& real_z2,
                                                     __m256& imag_z1,
                                                     __m256& imag_z2)
{
    const auto y_shared = _mm256_add_ps (shared_z1, _mm256_mul_ps (x, shared_b0));
    shared_z1 = _mm256_add_ps (shared_z2, _mm256_sub_ps (_mm256_mul_ps (x, shared_b1), _mm256_mul_ps (y_shared, a1)));
    shared_z2 = _mm256_sub_ps (_mm256_mul_ps (x, shared_b2), _mm256_mul_ps (y_shared, a2));

    const auto y_real = _mm256_add_ps (real_z1, y_shared); // for the real filter, we know that b[0] == 1
    real_z1 = _mm256_add_ps (real_z2, _mm256_sub_ps (_mm256_mul_ps (y_shared, real_b1), _mm256_mul_ps (y_real, a1)));
    real_z2 = _mm256_sub_ps (_mm256_mul_ps (y_shared, real_b2), _mm256_mul_ps (y_real, a2));

    const auto y_imag = imag_z1; // for the imaginary filter, we know that b[0] == 0
    imag_z1 = _mm256_add_ps (imag_z2, _mm256_sub_ps (_mm256_mul_ps (y_shared, imag_b1), _mm256_mul_ps (y_imag, a1)));
    imag_z2 = _mm256_sub_ps (_mm256_mul_ps (y_shared, imag_b2), _mm256_mul_ps (y_imag, a2));

    return { y_real, y_imag };
}
#endif
inline std::pair<__m128, __m128> process_sample_sse (__m128 x,
                                                     __m128 a1,
                                                     __m128 a2,
                                                     __m128 shared_b0,
                                                     __m128 shared_b1,
                                                     __m128 shared_b2,
                                                     __m128 real_b1,
                                                     __m128 real_b2,
                                                     __m128 imag_b1,
                                                     __m128 imag_b2,
                                                     __m128& shared_z1,
                                                     __m128& shared_z2,
                                                     __m128& real_z1,
                                                     __m128& real_z2,
                                                     __m128& imag_z1,
                                                     __m128& imag_z2)
{
    const auto y_shared = _mm_add_ps (shared_z1, _mm_mul_ps (x, shared_b0));
    shared_z1 = _mm_add_ps (shared_z2, _mm_sub_ps (_mm_mul_ps (x, shared_b1), _mm_mul_ps (y_shared, a1)));
    shared_z2 = _mm_sub_ps (_mm_mul_ps (x, shared_b2), _mm_mul_ps (y_shared, a2));

    const auto y_real = _mm_add_ps (real_z1, y_shared); // for the real filter, we know that b[0] == 1
    real_z1 = _mm_add_ps (real_z2, _mm_sub_ps (_mm_mul_ps (y_shared, real_b1), _mm_mul_ps (y_real, a1)));
    real_z2 = _mm_sub_ps (_mm_mul_ps (y_shared, real_b2), _mm_mul_ps (y_real, a2));

    const auto y_imag = imag_z1; // for the imaginary filter, we know that b[0] == 0
    imag_z1 = _mm_add_ps (imag_z2, _mm_sub_ps (_mm_mul_ps (y_shared, imag_b1), _mm_mul_ps (y_imag, a1)));
    imag_z2 = _mm_sub_ps (_mm_mul_ps (y_shared, imag_b2), _mm_mul_ps (y_imag, a2));

    return { y_real, y_imag };
}
#else
inline std::pair<float, float> process_sample_soa (float x,
                                                   float a1,
                                                   float a2,
                                                   float shared_b0,
                                                   float shared_b1,
                                                   float shared_b2,
                                                   float real_b1,
                                                   float real_b2,
                                                   float imag_b1,
                                                   float imag_b2,
                                                   float& shared_z1,
                                                   float& shared_z2,
                                                   float& real_z1,
                                                   float& real_z2,
                                                   float& imag_z1,
                                                   float& imag_z2)
{
    const auto y_shared = shared_z1 + x * shared_b0;
    shared_z1 = shared_z2 + x * shared_b1 - y_shared * a1;
    shared_z2 = x * shared_b2 - y_shared * a2;

    const auto y_real = real_z1 + y_shared; // for the real filter, we know that b[0] == 1
    real_z1 = real_z2 + y_shared * real_b1 - y_real * a1;
    real_z2 = y_shared * real_b2 - y_real * a2;

    const auto y_imag = imag_z1; // for the imaginary filter, we know that b[0] == 0
    imag_z1 = imag_z2 + y_shared * imag_b1 - y_imag * a1;
    imag_z2 = y_shared * imag_b2 - y_imag * a2;

    return { y_real, y_imag };
}
#endif

/** Computes the warping factor "K" so that the angular frequency wc is matched at sample rate fs */
template <typename T, typename NumericType>
inline T computeKValueAngular (T wc, NumericType fs)
{
    return wc / std::tan (wc / ((NumericType) 2 * fs));
}

/** Second-order bilinear transform */
template <typename T>
static void bilinear (T (&b)[3], T (&a)[3], const T (&bs)[3], const T (&as)[3], T K)
{
    // the mobius transform adds a little computational overhead, so here's the optimized bilinear transform.
    const auto KSq = K * K;

    const auto as0_KSq = as[0] * KSq;
    const auto as1_K = as[1] * K;
    const auto bs0_KSq = bs[0] * KSq;
    const auto bs1_K = bs[1] * K;

    const auto a0_inv = (T) 1 / (as0_KSq + as1_K + as[2]);

    a[0] = (T) 1;
    a[1] = (T) 2 * (as[2] - as0_KSq) * a0_inv;
    a[2] = (as0_KSq - as1_K + as[2]) * a0_inv;
    b[0] = (bs0_KSq + bs1_K + bs[2]) * a0_inv;
    b[1] = (T) 2 * (bs[2] - bs0_KSq) * a0_inv;
    b[2] = (bs0_KSq - bs1_K + bs[2]) * a0_inv;
}

/**
 * Calculates the filter coefficients for a given cutoff frequency, Q value, and sample rate.
 * The analog prototype transfer function is: \f$ H(s) = \frac{s/Q}{s^2 + s/Q + 1} \f$
 */
template <typename T, typename NumericType>
void calcSecondOrderBPF (T (&b)[3], T (&a)[3], T fc, T qVal, NumericType fs)
{
    const auto wc = (2.0f * static_cast<float> (M_PI)) * fc;
    const auto K = computeKValueAngular (wc, fs);
    auto kSqTerm = (T) 1 / (wc * wc);
    auto kTerm = (T) 1 / (qVal * wc);
    bilinear (b, a, { (T) 0, kTerm, (T) 0 }, { kSqTerm, kTerm, (T) 1 }, K);
}

static constexpr auto q_c = 4.0;
[[maybe_unused]] static auto design_erb_filter (size_t erb_index,
                                                double gamma,
                                                double erb_start,
                                                double q_ERB,
                                                double sample_rate,
                                                double (&b_coeffs_cplx_shared)[3],
                                                double (&b_coeffs_cplx_real)[3],
                                                double (&b_coeffs_cplx_imag)[3],
                                                double (&a_coeffs_cplx)[3])
{
    const auto q_PS = gamma;

    const auto z = erb_start + static_cast<double> (erb_index) * (q_c / q_ERB);
    const auto center_target_freq = 228.7 * (std::pow (10.0, z / 21.3) - 1.0);
    const auto filter_q = (1.0 / (q_PS * q_ERB)) * (24.7 + 0.108 * center_target_freq);

    double a_coeffs_proto[3];
    calcSecondOrderBPF (b_coeffs_cplx_shared,
                        a_coeffs_proto,
                        center_target_freq / gamma,
                        filter_q * 0.5,
                        sample_rate);

    auto pole = (std::sqrt (std::pow (std::complex { a_coeffs_proto[1] }, 2.0) - 4.0 * a_coeffs_proto[2]) - a_coeffs_proto[1]) / 2.0;
    if (std::imag (pole) < 0.0)
        pole = std::conj (pole);
    const auto pr = std::real (pole);
    const auto pi = std::imag (pole);

    // a[] = 1 - 2 pr z + (pi^2 + pr^2) z^2
    a_coeffs_cplx[0] = 1.0;
    a_coeffs_cplx[1] = -2.0 * pr;
    a_coeffs_cplx[2] = pi * pi + pr * pr;

    // b_real[] = 1 - 2 pr z + (-pi^2 + pr^2) z^2
    b_coeffs_cplx_real[0] = 1.0;
    b_coeffs_cplx_real[1] = -2.0 * pr;
    b_coeffs_cplx_real[2] = -pi * pi + pr * pr;

    // b_imag[] = 2 pi z - 2 pi pr z^2
    b_coeffs_cplx_imag[0] = 0.0;
    b_coeffs_cplx_imag[1] = 2.0 * pi;
    b_coeffs_cplx_imag[2] = -2.0 * pi * pr;

    return center_target_freq;
}

template <size_t N>
void design_filter_bank (std::array<ComplexERBFilterBank<N>, 2>& filter_bank,
                         double gamma,
                         double erb_start,
                         double q_ERB,
                         double sample_rate)
{
    for (auto& bank : filter_bank)
    {
        bank.shared_z1 = {};
        bank.shared_z2 = {};
        bank.imag_z1 = {};
        bank.imag_z2 = {};
        bank.real_z1 = {};
        bank.real_z2 = {};
    }

    for (size_t kiter = 0; kiter < ComplexERBFilterBank<N>::num_filter_bands; ++kiter)
    {
        double b_coeffs_cplx_shared_double[3] {};
        double b_coeffs_cplx_real_double[3] {};
        double b_coeffs_cplx_imag_double[3] {};
        double a_coeffs_cplx_double[3] {};
        design_erb_filter (kiter,
                           gamma,
                           erb_start,
                           q_ERB,
                           sample_rate,
                           b_coeffs_cplx_shared_double,
                           b_coeffs_cplx_real_double,
                           b_coeffs_cplx_imag_double,
                           a_coeffs_cplx_double);

        for (auto& bank : filter_bank)
        {
            bank.a1[kiter] = static_cast<float> (a_coeffs_cplx_double[1]);
            bank.a2[kiter] = static_cast<float> (a_coeffs_cplx_double[2]);
            bank.shared_b0[kiter] = static_cast<float> (b_coeffs_cplx_shared_double[0]);
            bank.shared_b1[kiter] = static_cast<float> (b_coeffs_cplx_shared_double[1]);
            bank.shared_b2[kiter] = static_cast<float> (b_coeffs_cplx_shared_double[2]);
            bank.real_b1[kiter] = static_cast<float> (b_coeffs_cplx_real_double[1]);
            bank.real_b2[kiter] = static_cast<float> (b_coeffs_cplx_real_double[2]);
            bank.imag_b1[kiter] = static_cast<float> (b_coeffs_cplx_imag_double[1]);
            bank.imag_b2[kiter] = static_cast<float> (b_coeffs_cplx_imag_double[2]);
        }
    }
}

/**
 * Returns the next pointer with a given byte alignment,
 * or the base pointer if it is already aligned.
 */
template <typename Type, typename IntegerType>
Type* snapPointerToAlignment (Type* basePointer,
                              IntegerType alignmentBytes) noexcept
{
    return (Type*) ((((size_t) basePointer) + (alignmentBytes - 1)) & ~(alignmentBytes - 1));
}

template <size_t num_octaves_up, size_t N>
void process (ComplexERBFilterBank<N>& filter_bank,
              const float* buffer_in,
              float* buffer_out,
              int num_samples) noexcept
{
    // buffer_out size is padded by 4x
    static constexpr auto eps = std::numeric_limits<float>::epsilon();
    static constexpr auto norm_gain = 2.0f / static_cast<float> (N);

#if defined(__ARM_NEON__)
    auto* buffer_out_simd = reinterpret_cast<float32x4_t*> (snapPointerToAlignment (buffer_out, 16));
    std::fill (buffer_out_simd, buffer_out_simd + num_samples, float32x4_t {});

    const auto eps_neon = vld1q_dup_f32 (&eps);
    const auto norm_gain_neon = vld1q_dup_f32 (&norm_gain);
    const auto zero_neon = float32x4_t {};
    for (size_t k = 0; k < N; k += 4)
    {
        const auto a1 = vld1q_f32 (filter_bank.a1.data() + k);
        const auto a2 = vld1q_f32 (filter_bank.a2.data() + k);
        const auto shared_b0 = vld1q_f32 (filter_bank.shared_b0.data() + k);
        const auto shared_b1 = vld1q_f32 (filter_bank.shared_b1.data() + k);
        const auto shared_b2 = vld1q_f32 (filter_bank.shared_b2.data() + k);
        const auto real_b1 = vld1q_f32 (filter_bank.real_b1.data() + k);
        const auto real_b2 = vld1q_f32 (filter_bank.real_b2.data() + k);
        const auto imag_b1 = vld1q_f32 (filter_bank.imag_b1.data() + k);
        const auto imag_b2 = vld1q_f32 (filter_bank.imag_b2.data() + k);

        auto shared_z1 = vld1q_f32 (filter_bank.shared_z1.data() + k);
        auto shared_z2 = vld1q_f32 (filter_bank.shared_z2.data() + k);
        auto real_z1 = vld1q_f32 (filter_bank.real_z1.data() + k);
        auto real_z2 = vld1q_f32 (filter_bank.real_z2.data() + k);
        auto imag_z1 = vld1q_f32 (filter_bank.imag_z1.data() + k);
        auto imag_z2 = vld1q_f32 (filter_bank.imag_z2.data() + k);

        for (int n = 0; n < num_samples; ++n)
        {
            const auto x_in = vld1q_dup_f32 (buffer_in + n);
            const auto [x_re, x_im] = process_sample_neon (x_in, a1, a2, shared_b0, shared_b1, shared_b2, real_b1, real_b2, imag_b1, imag_b2, shared_z1, shared_z2, real_z1, real_z2, imag_z1, imag_z2);

            auto x_re_sq = vmulq_f32 (x_re, x_re);
            auto x_im_sq = vmulq_f32 (x_im, x_im);
            auto x_abs_sq = vaddq_f32 (x_re_sq, x_im_sq);

            if constexpr (num_octaves_up == 1)
            {
                const auto greater_than_eps = vcgtq_f32 (x_abs_sq, eps_neon);
                const auto sqrt = vrsqrteq_f32 (x_abs_sq);
                const auto x_abs_r = vbslq_f32 (greater_than_eps, sqrt, zero_neon);
                buffer_out_simd[n] = vfmaq_f32 (buffer_out_simd[n], vsubq_f32 (x_re_sq, x_im_sq), x_abs_r);
            }
            else if constexpr (num_octaves_up == 2)
            {
                const auto greater_than_eps = vcgtq_f32 (x_abs_sq, eps_neon);
                const auto x_abs_r = vbslq_f32 (greater_than_eps, vrecpeq_f32 (x_abs_sq), zero_neon);
                const auto x_im_sq_x3 = vaddq_f32 (x_im_sq, vaddq_f32 (x_im_sq, x_im_sq));
                buffer_out_simd[n] = vfmaq_f32 (buffer_out_simd[n], vsubq_f32 (x_re_sq, x_im_sq_x3), vmulq_f32 (x_re, x_abs_r));

                // @TODO
                // auto x_abs_sq_r = xsimd::select (x_abs_sq > eps, xsimd::reciprocal (x_abs_sq), {});
                // buffer_out_simd[n] += x_re * (x_re_sq - (S) 3 * x_im_sq) * x_abs_sq_r;
            }
        }

        vst1q_f32 (filter_bank.shared_z1.data() + k, shared_z1);
        vst1q_f32 (filter_bank.shared_z2.data() + k, shared_z2);
        vst1q_f32 (filter_bank.real_z1.data() + k, real_z1);
        vst1q_f32 (filter_bank.real_z2.data() + k, real_z2);
        vst1q_f32 (filter_bank.imag_z1.data() + k, imag_z1);
        vst1q_f32 (filter_bank.imag_z2.data() + k, imag_z2);
    }

    for (int n = 0; n < num_samples; ++n)
    {
        buffer_out_simd[n] = vmulq_f32 (norm_gain_neon, buffer_out_simd[n]);
        auto rr = vadd_f32 (vget_high_f32 (buffer_out_simd[n]), vget_low_f32 (buffer_out_simd[n]));
        buffer_out[n] = vget_lane_f32 (vpadd_f32 (rr, rr), 0);
    }
#elif defined(__AVX__) || defined(__SSE2__)
    auto* buffer_out_simd = reinterpret_cast<__m128*> (snapPointerToAlignment (buffer_out, 16));
    std::fill (buffer_out_simd, buffer_out_simd + num_samples, __m128 {});

    const auto eps_sse = _mm_set1_ps (eps);
    const auto norm_gain_sse = _mm_set1_ps (norm_gain);
    for (size_t k = 0; k < N; k += 4)
    {
        const auto a1 = _mm_load_ps (filter_bank.a1.data() + k);
        const auto a2 = _mm_load_ps (filter_bank.a2.data() + k);
        const auto shared_b0 = _mm_load_ps (filter_bank.shared_b0.data() + k);
        const auto shared_b1 = _mm_load_ps (filter_bank.shared_b1.data() + k);
        const auto shared_b2 = _mm_load_ps (filter_bank.shared_b2.data() + k);
        const auto real_b1 = _mm_load_ps (filter_bank.real_b1.data() + k);
        const auto real_b2 = _mm_load_ps (filter_bank.real_b2.data() + k);
        const auto imag_b1 = _mm_load_ps (filter_bank.imag_b1.data() + k);
        const auto imag_b2 = _mm_load_ps (filter_bank.imag_b2.data() + k);

        auto shared_z1 = _mm_load_ps (filter_bank.shared_z1.data() + k);
        auto shared_z2 = _mm_load_ps (filter_bank.shared_z2.data() + k);
        auto real_z1 = _mm_load_ps (filter_bank.real_z1.data() + k);
        auto real_z2 = _mm_load_ps (filter_bank.real_z2.data() + k);
        auto imag_z1 = _mm_load_ps (filter_bank.imag_z1.data() + k);
        auto imag_z2 = _mm_load_ps (filter_bank.imag_z2.data() + k);

        for (int n = 0; n < num_samples; ++n)
        {
            const auto x_in = _mm_set1_ps (buffer_in[n]);
            const auto [x_re, x_im] = process_sample_sse (x_in, a1, a2, shared_b0, shared_b1, shared_b2, real_b1, real_b2, imag_b1, imag_b2, shared_z1, shared_z2, real_z1, real_z2, imag_z1, imag_z2);

            auto x_re_sq = _mm_mul_ps (x_re, x_re);
            auto x_im_sq = _mm_mul_ps (x_im, x_im);
            auto x_abs_sq = _mm_add_ps (x_re_sq, x_im_sq);

            if constexpr (num_octaves_up == 1)
            {
                const auto greater_than_eps = _mm_cmpgt_ps (x_abs_sq, eps_sse);
                const auto sqrt = _mm_rsqrt_ps (x_abs_sq);
                const auto x_abs_r = _mm_and_ps (greater_than_eps, sqrt);
                buffer_out_simd[n] = _mm_add_ps (buffer_out_simd[n], _mm_mul_ps (_mm_sub_ps (x_re_sq, x_im_sq), x_abs_r));
            }
            else if constexpr (num_octaves_up == 2)
            {
                const auto greater_than_eps = _mm_cmpgt_ps (x_abs_sq, eps_sse);
                const auto x_abs_sq_r = _mm_and_ps (greater_than_eps, _mm_rcp_ps (x_abs_sq));
                const auto x_im_sq_x3 = _mm_add_ps (x_im_sq, _mm_add_ps (x_im_sq, x_im_sq));
                buffer_out_simd[n] = _mm_add_ps (buffer_out_simd[n], _mm_mul_ps (_mm_sub_ps (x_re_sq, x_im_sq_x3), x_abs_sq_r));
            }
        }

        _mm_store_ps (filter_bank.shared_z1.data() + k, shared_z1);
        _mm_store_ps (filter_bank.shared_z2.data() + k, shared_z2);
        _mm_store_ps (filter_bank.real_z1.data() + k, real_z1);
        _mm_store_ps (filter_bank.real_z2.data() + k, real_z2);
        _mm_store_ps (filter_bank.imag_z1.data() + k, imag_z1);
        _mm_store_ps (filter_bank.imag_z2.data() + k, imag_z2);
    }

    for (int n = 0; n < num_samples; ++n)
    {
        buffer_out_simd[n] = _mm_mul_ps (norm_gain_sse, buffer_out_simd[n]);

        auto rr = _mm_add_ps (_mm_shuffle_ps (buffer_out_simd[n], buffer_out_simd[n], 0x4e), buffer_out_simd[n]);
        rr = _mm_add_ps (rr, _mm_shuffle_ps (rr, rr, 0xb1));
        buffer_out[n] = _mm_cvtss_f32 (rr);
    }
#else // No SIMD
    auto* buffer_out_simd = snapPointerToAlignment (reinterpret_cast<float*> (buffer_out), 4);
    std::fill (buffer_out_simd, buffer_out_simd + num_samples, float {});

    for (size_t k = 0; k < N; ++k)
    {
        const auto a1 = filter_bank.a1[k];
        const auto a2 = filter_bank.a2[k];
        const auto shared_b0 = filter_bank.shared_b0[k];
        const auto shared_b1 = filter_bank.shared_b1[k];
        const auto shared_b2 = filter_bank.shared_b2[k];
        const auto real_b1 = filter_bank.real_b1[k];
        const auto real_b2 = filter_bank.real_b2[k];
        const auto imag_b1 = filter_bank.imag_b1[k];
        const auto imag_b2 = filter_bank.imag_b2[k];

        auto shared_z1 = filter_bank.shared_z1[k];
        auto shared_z2 = filter_bank.shared_z2[k];
        auto real_z1 = filter_bank.real_z1[k];
        auto real_z2 = filter_bank.real_z2[k];
        auto imag_z1 = filter_bank.imag_z1[k];
        auto imag_z2 = filter_bank.imag_z2[k];

        for (int n = 0; n < num_samples; ++n)
        {
            const auto x_in = static_cast<float> (buffer_in[n]);
            const auto [x_re, x_im] = process_sample_soa (x_in,
                                                          a1,
                                                          a2,
                                                          shared_b0,
                                                          shared_b1,
                                                          shared_b2,
                                                          real_b1,
                                                          real_b2,
                                                          imag_b1,
                                                          imag_b2,
                                                          shared_z1,
                                                          shared_z2,
                                                          real_z1,
                                                          real_z2,
                                                          imag_z1,
                                                          imag_z2);

            auto x_re_sq = x_re * x_re;
            auto x_im_sq = x_im * x_im;
            auto x_abs_sq = x_re_sq + x_im_sq;

            if constexpr (num_octaves_up == 1)
            {
                auto x_abs_r = (x_abs_sq > eps) ? (1.0f / std::sqrt (x_abs_sq)) : 0.0f;
                buffer_out_simd[n] += (x_re_sq - x_im_sq) * x_abs_r;
            }
            else if constexpr (num_octaves_up == 2)
            {
                auto x_abs_sq_r = (x_abs_sq > eps) ? (1.0f / x_abs_sq) : 0.0f;
                buffer_out_simd[n] += x_re * (x_re_sq - 3.0f * x_im_sq) * x_abs_sq_r;
            }
        }

        filter_bank.shared_z1[k] = shared_z1;
        filter_bank.shared_z2[k] = shared_z2;
        filter_bank.real_z1[k] = real_z1;
        filter_bank.real_z2[k] = real_z2;
        filter_bank.imag_z1[k] = imag_z1;
        filter_bank.imag_z2[k] = imag_z2;
    }

    for (int n = 0; n < num_samples; ++n)
        buffer_out[n] = norm_gain * buffer_out_simd[n];
#endif
}

template <size_t num_octaves_up, size_t N>
void process_avx ([[maybe_unused]] ComplexERBFilterBank<N>& filter_bank,
                  [[maybe_unused]] const float* buffer_in,
                  [[maybe_unused]] float* buffer_out,
                  [[maybe_unused]] int num_samples) noexcept
{
    // buffer_out size is padded by 8x
    [[maybe_unused]] static constexpr auto eps = std::numeric_limits<float>::epsilon();
    [[maybe_unused]] static constexpr auto norm_gain = 2.0f / static_cast<float> (N);


#if defined(__AVX__)
    auto* buffer_out_simd = reinterpret_cast<__m256*> (snapPointerToAlignment (buffer_out, 32));
    std::fill (buffer_out_simd, buffer_out_simd + num_samples, __m256 {});

    const auto eps_avx = _mm256_set1_ps (eps);
    const auto norm_gain_avx = _mm256_set1_ps (norm_gain);
    const auto one_avx = _mm256_set1_ps (1.0f);
    for (size_t k = 0; k < N; k += 8)
    {
        const auto a1 = _mm256_load_ps (filter_bank.a1.data() + k);
        const auto a2 = _mm256_load_ps (filter_bank.a2.data() + k);
        const auto shared_b0 = _mm256_load_ps (filter_bank.shared_b0.data() + k);
        const auto shared_b1 = _mm256_load_ps (filter_bank.shared_b1.data() + k);
        const auto shared_b2 = _mm256_load_ps (filter_bank.shared_b2.data() + k);
        const auto real_b1 = _mm256_load_ps (filter_bank.real_b1.data() + k);
        const auto real_b2 = _mm256_load_ps (filter_bank.real_b2.data() + k);
        const auto imag_b1 = _mm256_load_ps (filter_bank.imag_b1.data() + k);
        const auto imag_b2 = _mm256_load_ps (filter_bank.imag_b2.data() + k);

        auto shared_z1 = _mm256_load_ps (filter_bank.shared_z1.data() + k);
        auto shared_z2 = _mm256_load_ps (filter_bank.shared_z2.data() + k);
        auto real_z1 = _mm256_load_ps (filter_bank.real_z1.data() + k);
        auto real_z2 = _mm256_load_ps (filter_bank.real_z2.data() + k);
        auto imag_z1 = _mm256_load_ps (filter_bank.imag_z1.data() + k);
        auto imag_z2 = _mm256_load_ps (filter_bank.imag_z2.data() + k);

        for (int n = 0; n < num_samples; ++n)
        {
            const auto x_in = _mm256_set1_ps (buffer_in[n]);
            const auto [x_re, x_im] = process_sample_avx (x_in, a1, a2, shared_b0, shared_b1, shared_b2, real_b1, real_b2, imag_b1, imag_b2, shared_z1, shared_z2, real_z1, real_z2, imag_z1, imag_z2);

            auto x_re_sq = _mm256_mul_ps (x_re, x_re);
            auto x_im_sq = _mm256_mul_ps (x_im, x_im);
            auto x_abs_sq = _mm256_add_ps (x_re_sq, x_im_sq);

            if constexpr (num_octaves_up == 1)
            {
                // const auto greater_than_eps = _mm256_cmpgt_ps (x_abs_sq, eps_avx);
                const auto greater_than_eps = _mm256_cmp_ps (x_abs_sq, eps_avx, _CMP_GT_OQ);
                const auto sqrt = _mm256_rsqrt_ps (x_abs_sq);
                const auto x_abs_r = _mm256_and_ps (greater_than_eps, sqrt);
                buffer_out_simd[n] = _mm256_add_ps (buffer_out_simd[n], _mm256_mul_ps (_mm256_sub_ps (x_re_sq, x_im_sq), x_abs_r));
            }
            else if constexpr (num_octaves_up == 2)
            {
                // const auto greater_than_eps = _mm256_cmpgt_ps (x_abs_sq, eps_avx);
                const auto greater_than_eps = _mm256_cmp_ps (x_abs_sq, eps_avx, _CMP_GT_OQ);
                const auto x_abs_sq_r = _mm256_and_ps (greater_than_eps, _mm256_rcp_ps (x_abs_sq));
                const auto x_im_sq_x3 = _mm256_add_ps (x_im_sq, _mm256_add_ps (x_im_sq, x_im_sq));
                buffer_out_simd[n] = _mm256_add_ps (buffer_out_simd[n], _mm256_mul_ps (_mm256_sub_ps (x_re_sq, x_im_sq_x3), x_abs_sq_r));
            }
        }

        _mm256_store_ps (filter_bank.shared_z1.data() + k, shared_z1);
        _mm256_store_ps (filter_bank.shared_z2.data() + k, shared_z2);
        _mm256_store_ps (filter_bank.real_z1.data() + k, real_z1);
        _mm256_store_ps (filter_bank.real_z2.data() + k, real_z2);
        _mm256_store_ps (filter_bank.imag_z1.data() + k, imag_z1);
        _mm256_store_ps (filter_bank.imag_z2.data() + k, imag_z2);
    }

    for (int n = 0; n < num_samples; ++n)
    {
        buffer_out_simd[n] = _mm256_mul_ps (norm_gain_avx, buffer_out_simd[n]);

        __m256 rr = _mm256_dp_ps (buffer_out_simd[n], one_avx, 0xff);
        __m256 tmp = _mm256_permute2f128_ps (rr, rr, 1);
        rr = _mm256_add_ps (rr, tmp);
        buffer_out[n] = _mm256_cvtss_f32  (rr);
    }
#endif
}

template void design_filter_bank<N1> (std::array<ComplexERBFilterBank<N1>, 2>&, double, double, double, double);
template void process<1, N1> (ComplexERBFilterBank<N1>&, const float*, float*, int) noexcept;
template void process<2, N1> (ComplexERBFilterBank<N1>&, const float*, float*, int) noexcept;
#if defined(__AVX__)
template void process_avx<1, N1> (ComplexERBFilterBank<N1>&, const float*, float*, int) noexcept;
template void process_avx<2, N1> (ComplexERBFilterBank<N1>&, const float*, float*, int) noexcept;
#endif
} // namespace poly_octave_v2
