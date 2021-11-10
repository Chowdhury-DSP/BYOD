#pragma once

#include "HysteresisOps.h"

/*
    Hysteresis processing for a model of an analog tape machine.
    For more information on the DSP happening here, see:
    https://ccrma.stanford.edu/~jatin/420/tape/TapeModel_DAFx.pdf
*/
class HysteresisProcessing
{
public:
    HysteresisProcessing() = default;

    void reset();
    void setSampleRate (double newSR);

    void setParameters (float drive, float width, float sat);

    void processBlock (double* bufferL, double* bufferR, const int numSamples);

private:
    // newton-raphson solvers
    template <int nIterations, typename Float>
    inline Float NRSolver (Float H, Float H_d) noexcept
    {
        using namespace chowdsp::SIMDUtils;

        Float M = M_n1;
        const Float last_dMdt = HysteresisOps::hysteresisFunc (M_n1, H_n1, H_d_n1, hpState);

        Float dMdt;
        Float dMdtPrime;
        Float deltaNR;
        for (int n = 0; n < nIterations; ++n)
        {
            dMdt = HysteresisOps::hysteresisFunc (M, H, H_d, hpState);
            dMdtPrime = HysteresisOps::hysteresisFuncPrime (H_d, dMdt, hpState);
            deltaNR = (M - M_n1 - (Float) Talpha * (dMdt + last_dMdt)) / (Float (1.0) - (Float) Talpha * dMdtPrime);
            M -= deltaNR;
        }

        return M;
    }

    void cook (float drive, float width, float sat);

    SmoothedValue<float, ValueSmoothingTypes::Linear> driveSmooth, satSmooth, widthSmooth;

    // parameter values
    double fs = 48000.0;
    double T = 1.0 / fs;
    double Talpha = T / 1.9;
    double upperLim = 20.0;

    // state variables
#if HYSTERESIS_USE_SIMD
    dsp::SIMDRegister<double> M_n1 = 0.0;
    dsp::SIMDRegister<double> H_n1 = 0.0;
    dsp::SIMDRegister<double> H_d_n1 = 0.0;
#else
    double M_n1 = 0.0;
    double H_n1 = 0.0;
    double H_d_n1 = 0.0;
#endif

    HysteresisOps::HysteresisState hpState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HysteresisProcessing)
};
