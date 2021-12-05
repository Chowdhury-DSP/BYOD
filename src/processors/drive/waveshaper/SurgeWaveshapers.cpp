/*
** Surge Synthesizer is Free and Open Source Software
**
** Surge is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Surge was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Surge
** open source in September 2018.
*/

#include "SurgeWaveshapers.h"

namespace SurgeWaveshapers
{
StringArray wst_names = {
    "Soft",
    "Hard",
    "Asymmetric",
    "Sine",
    "Digital",
    "Soft Harmonic 2",
    "Soft Harmonic 3",
    "Soft Harmonic 4",
    "Soft Harmonic 5",
    "Full Wave",
    "Half Wave Positive",
    "Half Wave Negative",
    "Soft Rectifier",
    "Single Fold",
    "Double Fold",
    "West Coast Fold",
    "Additive 1+2",
    "Additive 1+3",
    "Additive 1+4",
    "Additive 1+5",
    "Additive 12345",
    "Additive Saw 3",
    "Additive Square 3",

    "Fuzz",
    "Fuzz Soft Clip",
    "Heavy Fuzz",
    "Fuzz Center",
    "Fuzz Soft Edge",

    "Sin+x",
    "Sin 2x + x",
    "Sin 3x + x",
    "Sin 7x + x",
    "Sin 10x + x",
    "2 Cycle",
    "7 Cycle",
    "10 Cycle",
    "2 Cycle Bound",
    "7 Cycle Bound",
    "10 Cycle Bound",
    "Medium",
    "OJD",
    "Soft Single Fold"
};

using namespace chowdsp::SIMDUtils;

Vec4 CLIP (QuadFilterWaveshaperState* __restrict, Vec4 in, Vec4 drive)
{
    const Vec4 x_min = Vec4 (-1.0f);
    const Vec4 x_max = Vec4 (1.0f);
    return Vec4::max (Vec4::min ((in * drive), x_max), x_min);
}

Vec4 DIGI_SSE2 (QuadFilterWaveshaperState* __restrict, Vec4 in, Vec4 drive)
{
    // v1.2: return (double)((int)((double)(x*p0inv*16.f+1.0)))*p0*0.0625f;
    const Vec4 m16 = Vec4 (16.f);
    const Vec4 m16inv = Vec4 (0.0625f);
    const Vec4 mofs = Vec4 (0.5f);

    Vec4 invdrive = Vec4 (1.0f) / drive;
    Vec4 a = Vec4::truncate (mofs + (invdrive * (m16 * in)));

    return drive * (m16inv * (a - mofs));
}

Vec4 TANH (QuadFilterWaveshaperState* __restrict, Vec4 in, Vec4 drive)
{
    auto x = Vec4::max (Vec4::min ((in * drive), Vec4 (3.1f)), Vec4 (-3.1f));

    return x;
    //    return tanSIMD (x);
}

float Digi (const float x)
{
    constexpr int shift = 19;

    int32_t i = *(int32_t*) &x;
    i = i >> shift;
    i = i << shift;

    auto y = *(float*) &i;
    return y;
}

float Sinus (const float x) { return std::sin (x * (float) M_PI); }

inline Vec4 Asym (QuadFilterWaveshaperState* __restrict, Vec4 in, Vec4 drive)
{
    const auto tanh0p5 = Vec4 (0.46211715726f); // tanh(0.5)

    auto x = in * drive + Vec4 (0.5f);
    auto isPos = Vec4::greaterThanOrEqual (x, Vec4 (0.0f));
    auto pos = tanhSIMD (x);
    auto neg = chowdsp::Polynomials::horner<3> ({ Vec4 (0.07f), Vec4 (0.0054f), Vec4 (1.0f), Vec4 (0.0f) }, x);

    return ((pos & isPos) + (neg & (~isPos))) - tanh0p5;
}

template <int R1, int R2>
inline Vec4 dcBlock (QuadFilterWaveshaperState* __restrict s, Vec4 x)
{
    // https://www.dsprelated.com/freebooks/filters/DC_Blocker.html
    // y_n = x_n - x_n-1 + R y_n-1
    const auto fac = Vec4 (0.9999f);
    auto dx = x - s->R[R1];
    auto filtval = dx + (fac * s->R[R2]);
    s->R[R1] = x;
    s->R[R2] = filtval;
    s->init = dsp::SIMDRegister<uint32_t> (0);
    return filtval;
}

// Given a table of size N+1, N a power of 2, representing data between -1 and 1, interp
template <int N>
Vec4 WS_PM1_LUT (const float* table, Vec4 in)
{
    static const Vec4 one = Vec4 (1.f);
    static const Vec4 dx = Vec4 (N / 2.f);
    static const Vec4 ctr = Vec4 (N / 2.f);
    static const Vec4 UB = Vec4 (N - 1.f);
    static const Vec4 zero = Vec4 (0.0f);

    auto x = (in * dx) + ctr;
    auto e = Vec4::truncate (Vec4::max (Vec4::min (x, UB), zero));
    auto frac = x - e;

    // on PC write to memory & back as XMM -> GPR is slow on K8
    float e4 alignas (16)[4];
    e.copyToRawArray (e4);

    float wsArr alignas (16)[4];
    wsArr[0] = table[(int) e4[0]];
    wsArr[1] = table[(int) e4[1]];
    wsArr[2] = table[(int) e4[2]];
    wsArr[3] = table[(int) e4[3]];
    auto ws = Vec4::fromRawArray (wsArr);

    wsArr[0] = table[(int) e4[0] + 1];
    wsArr[1] = table[(int) e4[1] + 1];
    wsArr[2] = table[(int) e4[2] + 1];
    wsArr[3] = table[(int) e4[3] + 1];
    auto wsn = Vec4::fromRawArray (wsArr);

    auto res = ((one - frac) * ws) + (frac * wsn);

    return res;
}

template <int NP, float F (const float)>
struct LUTBase
{
    static constexpr int N = NP;
    static constexpr float dx = 2.0 / N;
    float data[N + 1];

    LUTBase()
    {
        for (int i = 0; i < N + 1; ++i)
        {
            float x = i * dx - 1.0f;
            data[i] = F (x);
        }
    }
};

template <int scale>
float FuzzTable (const float x)
{
    static auto gen = std::minstd_rand (2112);
    static const float range = 0.1f * scale;
    static auto dist = std::uniform_real_distribution<float> (-range, range);

    auto xadj = x * (1 - range) + dist (gen);
    return xadj;
}

template <int scale, Vec4 C (QuadFilterWaveshaperState* __restrict, Vec4, Vec4)>
Vec4 Fuzz (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    static LUTBase<1024, FuzzTable<scale>> table;
    return dcBlock<0, 1> (s, WS_PM1_LUT<1024> (table.data, C (s, x, drive)));
}

float FuzzCtrTable (const float x)
{
    static auto gen = std::minstd_rand (2112);
    static const float b = 20.0f;

    static auto dist = std::uniform_real_distribution<float> (-1.0, 1.0);

    auto g = exp (-x * x * b);
    auto xadj = x + g * dist (gen);
    return xadj;
}

float FuzzEdgeTable (const float x)
{
    static auto gen = std::minstd_rand (2112);
    static auto dist = std::uniform_real_distribution<float> (-1.0f, 1.0f);

    auto g = x * x * x * x;
    auto xadj = 0.85f * x + 0.15f * g * dist (gen);
    return xadj;
}

float SinPlusX (const float x) { return x - std::sin (x * (float) M_PI); }

template <int T>
float SinNXPlusXBound (const float x)
{
    auto z = 1 - fabs (x);
    auto r = z * std::sin (x * (float) M_PI * (float) T);
    return r + x;
}

template <int T>
float SinNX (const float x)
{
    return std::sin (x * (float) M_PI * (float) T);
}

template <int T>
float SinNXBound (const float x)
{
    auto z = 1 - fabs (x);
    auto r = z * std::sin (x * (float) M_PI * (float) T);
    return r;
}

template <float F (float), int N, Vec4 C (QuadFilterWaveshaperState* __restrict, Vec4, Vec4) = CLIP, bool block = true>
Vec4 TableEval (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    static LUTBase<N, F> table;
    if (block)
        return dcBlock<0, 1> (s, WS_PM1_LUT<N> (table.data, C (s, x, drive)));
    else
        return WS_PM1_LUT<N> (table.data, C (s, x, drive));
};

template <Vec4 (*K) (Vec4), bool useDCBlock>
Vec4 CHEBY_CORE (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    static const auto m1 = Vec4 (-1.0f);
    static const auto p1 = Vec4 (1.0f);

    auto bound = K (Vec4::max (Vec4::min (x, p1), m1));

    if (useDCBlock)
    {
        bound = dcBlock<0, 1> (s, bound);
    }
    return TANH (s, bound, drive);
}

Vec4 cheb2_kernel (Vec4 x) // 2 x^2 - 1
{
    static const auto m1 = Vec4 (1);
    static const auto m2 = Vec4 (2);
    return (m2 * (x * x)) - m1;
}

Vec4 cheb3_kernel (Vec4 x) // 4 x^3 - 3 x
{
    static const auto m4 = Vec4 (4);
    static const auto m3 = Vec4 (3);
    auto x2 = x * x;
    auto v4x2m3 = (m4 * x2) - m3;
    return v4x2m3 * x;
}

Vec4 cheb4_kernel (Vec4 x) // 8 x^4 - 8 x^2 + 1
{
    static const auto m1 = Vec4 (1);
    static const auto m8 = Vec4 (8);
    auto x2 = x * x;
    auto x4mx2 = x2 * (x2 - m1); // x^2 * ( x^2 - 1)
    return (m8 * x4mx2) + m1;
}

Vec4 cheb5_kernel (Vec4 x) // 16 x^5 - 20 x^3 + 5 x
{
    static const auto m16 = Vec4 (16);
    static const auto mn20 = Vec4 (-20);
    static const auto m5 = Vec4 (5);

    auto x2 = x * x;
    auto x3 = x2 * x;
    auto x5 = x3 * x2;
    auto t1 = m16 * x5;
    auto t2 = mn20 * x3;
    auto t3 = m5 * x;
    return t1 + (t2 + t3);
}

// Implement sum_n w_i T_i
template <int len>
struct ChebSeries
{
    ChebSeries (const std::initializer_list<float>& f)
    {
        auto q = f.begin();
        auto idx = 0;
        for (int i = 0; i < len; ++i)
            weights[i] = Vec4 (0.0f);
        while (idx < len && q != f.end())
        {
            weights[idx] = Vec4 (*q);
            ++idx;
            ++q;
        }
    }
    Vec4 weights[len];
    Vec4 eval (Vec4 x) const // x bound on 0,1
    {
        static const auto p2 = Vec4 (2.0);
        auto Tm1 = Vec4 (1.0);
        auto Tn = x;
        auto res = weights[0] + (weights[1] * x);
        for (int idx = 2; idx < len; ++idx)
        {
            auto nxt = ((p2 * Tn) * x) - Tm1;
            Tm1 = Tn;
            Tn = nxt;
            res = res + (weights[idx] * Tn);
        }
        return res;
    }
};

Vec4 Plus12 (QuadFilterWaveshaperState* __restrict s, Vec4 in, Vec4 drive)
{
    static const auto ser = ChebSeries<3> ({ 0.0f, 0.5f, 0.5f });
    static const auto scale = Vec4 (0.66f);
    return dcBlock<0, 1> (s, ser.eval (TANH (s, (in * scale), drive)));
}

Vec4 Plus13 (QuadFilterWaveshaperState* __restrict s, Vec4 in, Vec4 drive)
{
    static const auto ser = ChebSeries<4> ({ 0.0f, 0.5f, 0.0f, 0.5f });
    static const auto scale = Vec4 (0.66f);
    return ser.eval (TANH (s, (in * scale), drive));
}

Vec4 Plus14 (QuadFilterWaveshaperState* __restrict s, Vec4 in, Vec4 drive)
{
    static const auto ser = ChebSeries<5> ({ 0.0f, 0.5f, 0.0f, 0.0f, 0.5f });
    static const auto scale = Vec4 (0.66f);
    return dcBlock<0, 1> (s, ser.eval (TANH (s, (in * scale), drive)));
}

Vec4 Plus15 (QuadFilterWaveshaperState* __restrict s, Vec4 in, Vec4 drive)
{
    static const auto ser = ChebSeries<6> ({ 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f });
    static const auto scale = Vec4 (0.66f);
    return ser.eval (TANH (s, (in * scale), drive));
}

Vec4 Plus12345 (QuadFilterWaveshaperState* __restrict s, Vec4 in, Vec4 drive)
{
    static const auto ser = ChebSeries<6> ({ 0.0f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f });
    static const auto scale = Vec4 (0.66f);
    return dcBlock<0, 1> (s, ser.eval (TANH (s, (in * scale), drive)));
}

Vec4 PlusSaw3 (QuadFilterWaveshaperState* __restrict s, Vec4 in, Vec4 drive)
{
    static const float fac = 0.9f / (1.f + 0.5f + 0.25f);
    static const auto ser = ChebSeries<4> ({ 0.0f, -fac, fac * 0.5f, -fac * 0.25f });
    static const auto scale = Vec4 (-0.66f); // flip direction
    return dcBlock<0, 1> (s, ser.eval (TANH (s, (in * scale), drive)));
}

Vec4 PlusSqr3 (QuadFilterWaveshaperState* __restrict s, Vec4 in, Vec4 drive)
{
    static const float fac = 0.9f / (1.f - 0.25f + 1.0f / 16.0f);
    static const auto ser = ChebSeries<6> ({ 0.0f, fac, 0.0f, -fac * 0.25f, 0.0f, fac / 16.f });
    static const auto scale = Vec4 (0.66f);
    return ser.eval (TANH (s, (in * scale), drive));
}

/*
 * Given a function which is from x -> (F, adF) and two registers, do the ADAA
 * Set updateInit to false if you are going to wrap this in a dcBlocker (which
 * resets init itself) or other init adjusting function.
 */
template <void FandADF (Vec4, Vec4&, Vec4&), int xR, int aR, bool updateInit = true>
Vec4 ADAA (QuadFilterWaveshaperState* __restrict s, Vec4 x)
{
    auto xPrior = s->R[xR];
    auto adPrior = s->R[aR];

    Vec4 f, ad;
    FandADF (x, f, ad);

    auto dx = x - xPrior;
    auto dad = ad - adPrior;

    const static auto tolF = 0.0001f;
    const static auto tol = Vec4 (tolF), ntol = Vec4 (-tolF);
    auto ltt_i = Vec4::lessThan (dx, tol) & Vec4::greaterThan (dx, ntol); // dx < tol && dx > -tol
    auto ltt = ltt_i | s->init;
    auto dxDiv = Vec4 (1.0f) / ((tol & ltt) + (dx & (~ltt)));

    auto fFromAD = dad * dxDiv;
    auto r = (f & ltt) + (fFromAD & (~ltt));

    s->R[xR] = x;
    s->R[aR] = ad;
    if (updateInit)
    {
        s->init = dsp::SIMDRegister<uint32_t> (0);
    }

    return r;
}

void posrect_kernel (Vec4 x, Vec4& f, Vec4& adF)
{
    /*
     * F   : x > 0 ? x : 0
     * adF : x > 0 ? x^2/2 : 0
     *
     * observe that adF = F^2/2 in all cases
     */
    static const auto p5 = Vec4 (0.5f);
    auto gz = Vec4::greaterThanOrEqual (x, Vec4 (0.0f));
    f = x & gz;
    adF = p5 * (f * f);
}

template <int R1, int R2>
Vec4 ADAA_POS_WAVE (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    x = CLIP (s, x, drive);
    return ADAA<posrect_kernel, R1, R2> (s, x);
}

void negrect_kernel (Vec4 x, Vec4& f, Vec4& adF)
{
    /*
     * F   : x < 0 ? x : 0
     * adF : x < 0 ? x^2/2 : 0
     *
     * observe that adF = F^2/2 in all cases
     */
    static const auto p5 = Vec4 (0.5f);
    auto gz = Vec4::lessThanOrEqual (x, Vec4 (0.0f));
    f = x & gz;
    adF = p5 * (f * f);
}

template <int R1, int R2>
Vec4 ADAA_NEG_WAVE (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    x = CLIP (s, x, drive);
    return ADAA<negrect_kernel, R1, R2> (s, x);
}

void fwrect_kernel (Vec4 x, Vec4& F, Vec4& adF)
{
    /*
     * F   : x > 0 ? x : -x  = sgn(x) * x
     * adF : x > 0 ? x^2 / 2 : -x^2 / 2 = sgn(x) * x^2 / 2
     */
    static const auto p1 = Vec4 (1.f);
    static const auto p05 = Vec4 (0.5f);
    auto gz = Vec4::greaterThanOrEqual (x, Vec4 (0.0f));
    auto sgn = (p1 & gz) - (p1 & (~gz));

    F = sgn * x;
    adF = F * (x * p05); // sgn * x * x * 0.5
}

Vec4 ADAA_FULL_WAVE (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    x = CLIP (s, x, drive);
    return ADAA<fwrect_kernel, 0, 1> (s, x);
}

void softrect_kernel (Vec4 x, Vec4& F, Vec4& adF)
{
    /*
     * F   : x > 0 ? 2x-1 : -2x - 1   = 2 * ( sgn(x) * x ) - 1
     * adF : x > 0 ? x^2-x : -x^2 - x = sgn(x) * x^2 - x
     */
    static const auto p2 = Vec4 (2.f);
    static const auto p1 = Vec4 (1.f);
    auto gz = Vec4::greaterThanOrEqual (x, Vec4 (0.0f));
    auto sgn = (p1 & gz) - (p1 & (~gz));

    F = (p2 * (sgn * x)) - p1;
    adF = (sgn * (x * x)) - x;
}

Vec4 ADAA_SOFTRECT_WAVE (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    return TANH (s, dcBlock<2, 3> (s, ADAA<softrect_kernel, 0, 1, false> (s, x)), drive);
}

template <int pts>
struct FolderADAA
{
    FolderADAA (std::initializer_list<float> xi, std::initializer_list<float> yi)
    {
        auto xiv = xi.begin();
        auto yiv = yi.begin();
        for (int i = 0; i < pts; ++i)
        {
            xs[i] = *xiv;
            ys[i] = *yiv;

            xiv++;
            yiv++;
        }

        slopes[pts - 1] = 0;
        dxs[pts - 1] = 0;

        intercepts[0] = -xs[0] * ys[0];
        for (int i = 0; i < pts - 1; ++i)
        {
            dxs[i] = xs[i + 1] - xs[i];
            slopes[i] = (ys[i + 1] - ys[i]) / dxs[i];
            auto vLeft = slopes[i] * dxs[i] * dxs[i] / 2 + ys[i] * xs[i + 1] + intercepts[i];
            auto vRight = ys[i + 1] * xs[i + 1];
            intercepts[i + 1] = -vRight + vLeft;
        }

        for (int i = 0; i < pts; ++i)
        {
            xS[i] = Vec4 (xs[i]);
            yS[i] = Vec4 (ys[i]);
            mS[i] = Vec4 (slopes[i]);
            cS[i] = Vec4 (intercepts[i]);
        }
    }

    inline void evaluate (Vec4 x, Vec4& f, Vec4& adf)
    {
        static const auto p05 = Vec4 (0.5f);
        Vec4 val[pts - 1], adVal[pts - 1];
        dsp::SIMDRegister<uint32_t> rangeMask[pts - 1];

        for (int i = 0; i < pts - 1; ++i)
        {
            rangeMask[i] = Vec4::greaterThanOrEqual (x, xS[i]) & Vec4::lessThan (x, xS[i + 1]);
            auto ox = x - xS[i];
            val[i] = (mS[i] * ox) + yS[i];
            adVal[i] = ((ox * ox) * (mS[i] * p05)) + ((yS[i] * x) + cS[i]);
        }
        auto res = val[0] & rangeMask[0];
        auto adres = adVal[0] & rangeMask[0];
        for (int i = 1; i < pts - 1; ++i)
        {
            res = res + (val[i] & rangeMask[i]);
            adres = adres + (adVal[i] & rangeMask[i]);
        }
        f = res;
        adf = adres;
    }
    float xs[pts], ys[pts], dxs[pts], slopes[pts], intercepts[pts];

    Vec4 xS[pts], yS[pts], dxS[pts], mS[pts], cS[pts];
};

void singleFoldADAA (Vec4 x, Vec4& f, Vec4& adf)
{
    static auto folder = FolderADAA<4> ({ -10.0f, -0.7f, 0.7f, 10.0f }, { -1.0f, 1.0f, -1.0f, 1.0f });
    folder.evaluate (x, f, adf);
}

void dualFoldADAA (Vec4 x, Vec4& f, Vec4& adf)
{
    static auto folder =
        FolderADAA<8> ({ -10.0f, -3.0f, -1.0f, -0.3f, 0.3f, 1.0f, 3.0f, 10.0f }, { -1.0f, -0.9f, 1.0f, -1.0f, 1.0f, -1.0f, 0.9f, 1.0f });
    folder.evaluate (x, f, adf);
}

void westCoastFoldADAA (Vec4 x, Vec4& f, Vec4& adf)
{
    // Factors based on
    // DAFx-17 DAFX-194 Virtual Analog Buchla 259 Wavefolder
    // by Sequeda, Pontynen, Valimaki and Parker
    static auto folder = FolderADAA<14> (
        { -10.0f, -2.0f, -1.0919091909190919f, -0.815881588158816f, -0.5986598659865987f, -0.3598359835983597f, -0.11981198119811971f, 0.11981198119811971f, 0.3598359835983597f, 0.5986598659865987f, 0.8158815881588157f, 1.0919091909190919f, 2.0f, 10.0f },
        { 1.0f, 0.9f, -0.679765619488133f, 0.5309659972270625f, -0.6255506631744251f, 0.5991799179917987f, -0.5990599059905986f, 0.5990599059905986f, -0.5991799179917987f, 0.6255506631744251f, -0.5309659972270642f, 0.679765619488133f, -0.9f, -1.0f });
    folder.evaluate (x, f, adf);
}

template <void F (Vec4, Vec4&, Vec4&)>
Vec4 WAVEFOLDER (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    x = x * drive;
    return ADAA<F, 0, 1> (s, x);
}

Vec4 ZAMSAT (QuadFilterWaveshaperState* __restrict s, Vec4 x, Vec4 drive)
{
    x = CLIP (s, x, drive);

    // 2x * ( 1 - |x| )
    static const auto p1 = Vec4 (1.f);
    static const auto p2 = Vec4 (2.f);

    auto gz = Vec4::greaterThanOrEqual (x, Vec4 (0.0f));
    auto sgn = (p1 & gz) - (p1 & (~gz));

    auto twox = p2 * x;
    auto F = twox - (sgn * (x * x));
    return F;
}

Vec4 SoftOneFold (QuadFilterWaveshaperState* __restrict, Vec4 x, Vec4 drive)
{
    // x / (0.4 + 0.7 x^2)
    auto y = x * drive;
    auto y2 = y * y;

    static const auto p04 = Vec4 (0.4f);
    static const auto p07 = Vec4 (0.7f);

    auto num = p04 + (p07 * y2);

    return y * (Vec4 (1.0f) / num);
}

Vec4 OJD (QuadFilterWaveshaperState* __restrict, Vec4 x, Vec4 drive)
{
    x = x * drive;

    static const auto pm17 = Vec4 (-1.7f);
    static const auto p11 = Vec4 (1.1f);
    static const auto pm03 = Vec4 (-0.3f);
    static const auto p09 = Vec4 (0.9f);

    static const auto denLow = Vec4 (1.f / (4 * (1 - 0.3f)));
    static const auto denHigh = Vec4 (1.f / (4 * (1 - 0.9f)));

    auto maskNeg = Vec4::lessThanOrEqual (x, pm17); // in <= -1.7f
    auto maskPos = Vec4::greaterThanOrEqual (x, p11); // in > 1.1f
    auto maskLow = (~maskNeg) & Vec4::lessThan (x, pm03); // in > -1.7 && in < =0.3
    auto maskHigh = (~maskPos) & Vec4::greaterThan (x, p09); // in > 0.9 && in < 1.1
    auto maskMid = Vec4::greaterThanOrEqual (x, pm03) & Vec4::lessThanOrEqual (x, p09); // the middle

    static const auto vNeg = Vec4 (-1.0);
    static const auto vPos = Vec4 (1.0);
    auto vMid = x;

    auto xlow = x - pm03;
    auto vLow = xlow + (denLow * (xlow * xlow));
    vLow = vLow + pm03;

    auto xhi = x - p09;
    auto vHi = xhi - (denHigh * (xhi * xhi));
    vHi = vHi + p09;

    auto res =
        (((vNeg & maskNeg) + (vLow & maskLow)) + ((vHi & maskHigh) + (vPos & maskPos))) + (vMid & maskMid);

    return res;
}

WaveshaperQFPtr GetQFPtrWaveshaper (int type)
{
    switch (type)
    {
        case wst_soft:
            return TANH;
        case wst_hard:
            return CLIP;
        case wst_asym:
            return Asym;
        case wst_sine:
            return TableEval<Sinus, 1024, CLIP, false>;
        case wst_digital:
            return TableEval<Digi, 2048, CLIP, false>;
            //            return DIGI_SSE2;
        case wst_cheby2:
            return CHEBY_CORE<cheb2_kernel, true>;
        case wst_cheby3:
            return CHEBY_CORE<cheb3_kernel, false>;
        case wst_cheby4:
            return CHEBY_CORE<cheb4_kernel, true>;
        case wst_cheby5:
            return CHEBY_CORE<cheb5_kernel, false>;
        case wst_fwrectify:
            return ADAA_FULL_WAVE;
        case wst_softrect:
            return ADAA_SOFTRECT_WAVE;
        case wst_poswav:
            return ADAA_POS_WAVE<0, 1>;
        case wst_negwav:
            return ADAA_NEG_WAVE<0, 1>;
        case wst_singlefold:
            return WAVEFOLDER<singleFoldADAA>;
        case wst_dualfold:
            return WAVEFOLDER<dualFoldADAA>;
        case wst_westfold:
            return WAVEFOLDER<westCoastFoldADAA>;
        case wst_add12:
            return Plus12;
        case wst_add13:
            return Plus13;
        case wst_add14:
            return Plus14;
        case wst_add15:
            return Plus15;
        case wst_add12345:
            return Plus12345;
        case wst_addsaw3:
            return PlusSaw3;
        case wst_addsqr3:
            return PlusSqr3;
        case wst_fuzz:
            return TableEval<FuzzTable<1>, 1024>;
        case wst_fuzzsoft:
            return TableEval<FuzzTable<1>, 1024, TANH>;
        case wst_fuzzheavy:
            return TableEval<FuzzTable<3>, 1024>;
        case wst_fuzzctr:
            return TableEval<FuzzCtrTable, 2048, TANH>;
        case wst_fuzzsoftedge:
            return TableEval<FuzzEdgeTable, 2048, TANH>;

        case wst_sinpx:
            return TableEval<SinPlusX, 1024, CLIP, false>;

        case wst_sin2xpb:
            return TableEval<SinNXPlusXBound<2>, 2048, CLIP, false>;
        case wst_sin3xpb:
            return TableEval<SinNXPlusXBound<3>, 2048, CLIP, false>;
        case wst_sin7xpb:
            return TableEval<SinNXPlusXBound<7>, 2048, CLIP, false>;
        case wst_sin10xpb:
            return TableEval<SinNXPlusXBound<10>, 2048, CLIP, false>;

        case wst_2cyc:
            return TableEval<SinNX<2>, 2048, CLIP, false>;
        case wst_7cyc:
            return TableEval<SinNX<7>, 2048, CLIP, false>;
        case wst_10cyc:
            return TableEval<SinNX<10>, 2048, CLIP, false>;
        case wst_2cycbound:
            return TableEval<SinNXBound<2>, 2048, CLIP, false>;
        case wst_7cycbound:
            return TableEval<SinNXBound<7>, 2048, CLIP, false>;
        case wst_10cycbound:
            return TableEval<SinNXBound<10>, 2048, CLIP, false>;

        case wst_zamsat:
            return ZAMSAT;
        case wst_ojd:
            return OJD;
        case wst_softfold:
            return SoftOneFold;
    }
    return 0;
}

void initializeWaveshaperRegister (int type, float R[n_waveshaper_registers])
{
    switch (type)
    {
        default:
        {
            for (int i = 0; i < n_waveshaper_registers; ++i)
                R[i] = 0.f;
        }
        break;
    }
    return;
}
} // namespace SurgeWaveshapers
