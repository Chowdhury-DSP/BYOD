#ifndef HYSTERESISPROCESSING_H_INCLUDED
#define HYSTERESISPROCESSING_H_INCLUDED

#include <pch.h>

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

    void processBlock (double* buffer, const int numSamples);

    // parameter values
    double fs = 48000.0;
    double T = 1.0 / fs;
    double Talpha = T / 1.9;
    double M_s = 1.0;
    double a = M_s / 4.0;
    const double alpha = 1.6e-3;
    double k = 0.47875;
    double c = 1.7e-1;
    double upperLim = 20.0;

    // Save calculations
    double nc = 1 - c;
    double M_s_oa = M_s / a;
    double M_s_oa_talpha = alpha * M_s / a;
    double M_s_oa_tc = c * M_s / a;
    double M_s_oa_tc_talpha = alpha * c * M_s / a;
    double M_s_oaSq_tc_talpha = alpha * c * M_s / (a * a);
    double M_s_oaSq_tc_talphaSq = alpha * alpha * c * M_s / (a * a);

    // state variables
    double M_n1 = 0.0;
    double H_n1 = 0.0;
    double H_d_n1 = 0.0;

    // temp vars
    double Q, M_diff, delta, delta_M, L_prime, kap1, f1Denom, f1, f2, f3;
    double coth = 0.0;
    bool nearZero = false;

private:
    void cook (float drive, float width, float sat);

    SmoothedValue<float, ValueSmoothingTypes::Linear> driveSmooth, satSmooth, widthSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HysteresisProcessing)
};

#endif
