#include "LowPassLadder.h"

void LowPassLadder::reset (const double _sample_rate)
{
    sample_rate = _sample_rate;

    for (int i = 0; i < 4; ++i)
    {
        lp[i].reset (sample_rate);
    }
}

double LowPassLadder::get_sample_rate() const
{
    return sample_rate;
}

//==============================================================================

void LowPassLadder::set_cutoff (const double cutoff)
{
    const double omega_digital = ladder_filter_utility::TWO_PI * cutoff;
    const double omega_analog = ladder_filter_utility::prewarp (omega_digital, sample_rate);

    g = omega_analog / (2.0 * sample_rate);
    G = g / (1.0 + g);
    G2 = G * G;
    G3 = G2 * G;
    G4 = G3 * G;

    for (int i = 0; i < 4; ++i)
    {
        lp[i].set_G (G);
    }
}

void LowPassLadder::set_resonance (const double resonance)
{
    k = ladder_filter_utility::map_linear_normalized (resonance, 0.0, 4.0);
}

//==============================================================================

double LowPassLadder::process (double x)
{
    const double s1 = lp[0].get_state();
    const double s2 = lp[1].get_state();
    const double s3 = lp[2].get_state();
    const double s4 = lp[3].get_state();

    const double S = (G3 * s1 + G2 * s2 + G * s3 + s4) / (1.0 + g);
    double y = (x - k * S) / (1.0 + k * G4);

    for (int i = 0; i < 4; ++i)
    {
        y = lp[i].process (y);
    }

    // Compensate for preceived volume loss with higher resonance settings
    return y * (1.0 + k);
}