#include "HysteresisProcessing.h"
#include <math.h>

namespace
{
constexpr double ONE_THIRD = 1.0 / 3.0;
constexpr double NEG_TWO_OVER_15 = -2.0 / 15.0;
} // namespace

inline int sign (double x)
{
    return (x > 0.0) - (x < 0.0);
}

void HysteresisProcessing::reset()
{
    M_n1 = 0.0;
    H_n1 = 0.0;
    H_d_n1 = 0.0;

    coth = 0.0;
    nearZero = false;
}

void HysteresisProcessing::setSampleRate (double newSR)
{
    fs = newSR;
    T = 1.0 / fs;
    Talpha = T / 1.9;

    driveSmooth.reset (newSR, 0.01);
    satSmooth.reset (newSR, 0.01);
    widthSmooth.reset (newSR, 0.01);
}

void HysteresisProcessing::setParameters (float drive, float width, float sat)
{
    driveSmooth.setTargetValue (drive);
    satSmooth.setTargetValue (sat);
    widthSmooth.setTargetValue (width);
}

void HysteresisProcessing::cook (float drive, float width, float sat)
{
    M_s = 0.5 + 1.5 * (1.0 - (double) sat);
    a = M_s / (0.01 + 6.0 * (double) drive);
    c = std::sqrt (1.0f - (double) width) - 0.01;
    k = 0.47875;
    upperLim = 20.0;

    nc = 1.0 - c;
    M_s_oa = M_s / a;
    M_s_oa_talpha = alpha * M_s_oa;
    M_s_oa_tc = c * M_s_oa;
    M_s_oa_tc_talpha = alpha * M_s_oa_tc;
    M_s_oaSq_tc_talpha = M_s_oa_tc_talpha / a;
    M_s_oaSq_tc_talphaSq = alpha * M_s_oaSq_tc_talpha;
}

static inline double langevin (double x, const HysteresisProcessing& hp) noexcept // Langevin function
{
    return (! hp.nearZero) ? (hp.coth) - (1.0 / x) : x / 3.0;
}

static inline double langevinD (double x, const HysteresisProcessing& hp) noexcept // Derivative of Langevin function
{
    return (! hp.nearZero) ? (1.0 / (x * x)) - (hp.coth * hp.coth) + 1.0 : ONE_THIRD;
}

static inline double langevinD2 (double x, const HysteresisProcessing& hp) noexcept // 2nd derivative of Langevin function
{
    return (! hp.nearZero) ? 2.0 * hp.coth * (hp.coth * hp.coth - 1.0) - (2.0 / (x * x * x)) : NEG_TWO_OVER_15 * x;
}

static inline double deriv (double x_n, double x_n1, double x_d_n1, const HysteresisProcessing& hp) noexcept // Derivative by alpha transform
{
    constexpr double dAlpha = 0.75;
    return (((1.0 + dAlpha) / hp.T) * (x_n - x_n1)) - dAlpha * x_d_n1;
}

// hysteresis function dM/dt
static inline double hysteresisFunc (double M, double H, double H_d, HysteresisProcessing& hp) noexcept
{
    hp.Q = (H + hp.alpha * M) / hp.a;
    hp.coth = 1.0 / std::tanh (hp.Q);
    hp.nearZero = hp.Q < 0.001 && hp.Q > -0.001;

    hp.M_diff = hp.M_s * langevin (hp.Q, hp) - M;

    hp.delta = (double) ((H_d >= 0.0) - (H_d < 0.0));
    hp.delta_M = (double) (sign (hp.delta) == sign (hp.M_diff));

    hp.L_prime = langevinD (hp.Q, hp);

    hp.kap1 = hp.nc * hp.delta_M;
    hp.f1Denom = hp.nc * hp.delta * hp.k - hp.alpha * hp.M_diff;
    hp.f1 = hp.kap1 * hp.M_diff / hp.f1Denom;
    hp.f2 = hp.M_s_oa_tc * hp.L_prime;
    hp.f3 = 1.0 - (hp.M_s_oa_tc_talpha * hp.L_prime);

    return H_d * (hp.f1 + hp.f2) / hp.f3;
}

// derivative of hysteresis func w.r.t M (depends on cached values from computing hysteresisFunc)
static inline double hysteresisFuncPrime (double H_d, double dMdt, const HysteresisProcessing& hp) noexcept
{
    const double L_prime2 = langevinD2 (hp.Q, hp);
    const double M_diff2 = hp.M_s_oa_talpha * hp.L_prime - 1.0;

    const double f1_p = hp.kap1 * ((M_diff2 / hp.f1Denom) + hp.M_diff * hp.alpha * M_diff2 / (hp.f1Denom * hp.f1Denom));
    const double f2_p = hp.M_s_oaSq_tc_talpha * L_prime2;
    const double f3_p = -hp.M_s_oaSq_tc_talphaSq * L_prime2;

    return H_d * (f1_p + f2_p) / hp.f3 - dMdt * f3_p / hp.f3;
}

static inline double NewtonRaphson (double H, double H_d, HysteresisProcessing& hp) noexcept
{
    double M = hp.M_n1;
    const double last_dMdt = hysteresisFunc (hp.M_n1, hp.H_n1, hp.H_d_n1, hp);

    double dMdt, dMdtPrime, deltaNR;
    {
        // loop #1
        dMdt = hysteresisFunc (M, H, H_d, hp);
        dMdtPrime = hysteresisFuncPrime (H_d, dMdt, hp);
        deltaNR = (M - hp.M_n1 - hp.Talpha * (dMdt + last_dMdt)) / (1.0 - hp.Talpha * dMdtPrime);
        M -= deltaNR;

        // loop #2
        dMdt = hysteresisFunc (M, H, H_d, hp);
        dMdtPrime = hysteresisFuncPrime (H_d, dMdt, hp);
        deltaNR = (M - hp.M_n1 - hp.Talpha * (dMdt + last_dMdt)) / (1.0 - hp.Talpha * dMdtPrime);
        M -= deltaNR;

        // loop #3
        dMdt = hysteresisFunc (M, H, H_d, hp);
        dMdtPrime = hysteresisFuncPrime (H_d, dMdt, hp);
        deltaNR = (M - hp.M_n1 - hp.Talpha * (dMdt + last_dMdt)) / (1.0 - hp.Talpha * dMdtPrime);
        M -= deltaNR;

        // loop #4
        dMdt = hysteresisFunc (M, H, H_d, hp);
        dMdtPrime = hysteresisFuncPrime (H_d, dMdt, hp);
        deltaNR = (M - hp.M_n1 - hp.Talpha * (dMdt + last_dMdt)) / (1.0 - hp.Talpha * dMdtPrime);
        M -= deltaNR;
    }

    return M;
}

void HysteresisProcessing::processBlock (double* buffer, const int numSamples)
{
    bool needsSmoothing = driveSmooth.isSmoothing() || widthSmooth.isSmoothing() || satSmooth.isSmoothing();

    if (needsSmoothing)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            cook (driveSmooth.getNextValue(), widthSmooth.getNextValue(), satSmooth.getNextValue());

            auto H = buffer[n];
            double H_d = deriv (H, H_n1, H_d_n1, *this);
            double M = NewtonRaphson (H, H_d, *this);

            // check for instability
            bool illCondition = std::isnan (M) || M > upperLim;
            M = illCondition ? 0.0 : M;
            H_d = illCondition ? 0.0 : H_d;

            M_n1 = M;
            H_n1 = H;
            H_d_n1 = H_d;

            buffer[n] = M;
        }
    }
    else
    {
        for (int n = 0; n < numSamples; ++n)
        {
            auto H = buffer[n];
            double H_d = deriv (H, H_n1, H_d_n1, *this);
            double M = NewtonRaphson (H, H_d, *this);

            // check for instability
            bool illCondition = std::isnan (M) || M > upperLim;
            M = illCondition ? 0.0 : M;
            H_d = illCondition ? 0.0 : H_d;

            M_n1 = M;
            H_n1 = H;
            H_d_n1 = H_d;

            buffer[n] = M;
        }
    }
}
