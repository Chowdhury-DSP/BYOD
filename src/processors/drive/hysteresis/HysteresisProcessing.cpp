#include "HysteresisProcessing.h"

void HysteresisProcessing::reset()
{
    M_n1 = 0.0;
    H_n1 = 0.0;
    H_d_n1 = 0.0;

    hpState.coth = 0.0;
    hpState.nearZero = false;
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
    hpState.M_s = 0.5 + 1.5 * (1.0 - (double) sat);
    hpState.a = hpState.M_s / (0.01 + 6.0 * (double) drive);
    hpState.c = std::sqrt (1.0f - (double) width) - 0.01;
    hpState.k = 0.47875;
    upperLim = 20.0;

    hpState.nc = 1.0 - hpState.c;
    hpState.M_s_oa = hpState.M_s / hpState.a;
    hpState.M_s_oa_talpha = hpState.alpha * hpState.M_s_oa;
    hpState.M_s_oa_tc = hpState.c * hpState.M_s_oa;
    hpState.M_s_oa_tc_talpha = hpState.alpha * hpState.M_s_oa_tc;
    hpState.M_s_oaSq_tc_talpha = hpState.M_s_oa_tc_talpha / hpState.a;
    hpState.M_s_oaSq_tc_talphaSq = hpState.alpha * hpState.M_s_oaSq_tc_talpha;
}

void HysteresisProcessing::processBlock (double* bufferLeft, double* bufferRight, const int numSamples)
{
    using Float = dsp::SIMDRegister<double>;

    bool needsSmoothing = driveSmooth.isSmoothing() || widthSmooth.isSmoothing() || satSmooth.isSmoothing();

    double stereoVec alignas (16)[2];

    if (needsSmoothing)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            cook (driveSmooth.getNextValue(), widthSmooth.getNextValue(), satSmooth.getNextValue());

            stereoVec[0] = bufferLeft[n];
            stereoVec[1] = bufferRight[n];
            auto H = Float::fromRawArray (stereoVec);
            auto H_d = HysteresisOps::deriv (H, H_n1, H_d_n1, (Float) T);
            auto M = NRSolver<4> (H, H_d);

            // check for instability
#if HYSTERESIS_USE_SIMD
            auto notIllCondition = ~(chowdsp::SIMDUtils::isnanSIMD (M) | Float::greaterThan (M, (Float) upperLim));
            M = M & notIllCondition;
            H_d = H_d & notIllCondition;
#else
            bool illCondition = std::isnan (M) || M > upperLim;
            M = illCondition ? 0.0 : M;
            H_d = illCondition ? 0.0 : H_d;
#endif

            M_n1 = M;
            H_n1 = H;
            H_d_n1 = H_d;

            M.copyToRawArray (stereoVec);
            bufferLeft[n] = stereoVec[0];
            bufferRight[n] = stereoVec[1];
        }
    }
    else
    {
        for (int n = 0; n < numSamples; ++n)
        {
            stereoVec[0] = bufferLeft[n];
            stereoVec[1] = bufferRight[n];
            auto H = Float::fromRawArray (stereoVec);
            auto H_d = HysteresisOps::deriv (H, H_n1, H_d_n1, (Float) T);
            auto M = NRSolver<4> (H, H_d);

            // check for instability
#if HYSTERESIS_USE_SIMD
            auto notIllCondition = ~(chowdsp::SIMDUtils::isnanSIMD (M) | Float::greaterThan (M, (Float) upperLim));
            M = M & notIllCondition;
            H_d = H_d & notIllCondition;
#else
            bool illCondition = std::isnan (M) || M > upperLim;
            M = illCondition ? 0.0 : M;
            H_d = illCondition ? 0.0 : H_d;
#endif

            M_n1 = M;
            H_n1 = H;
            H_d_n1 = H_d;

            M.copyToRawArray (stereoVec);
            bufferLeft[n] = stereoVec[0];
            bufferRight[n] = stereoVec[1];
        }
    }
}
