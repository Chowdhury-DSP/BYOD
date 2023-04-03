#include "HighPassLadder.h"

void HighPassLadder::reset (const double _sample_rate)
{
    sample_rate = _sample_rate;

    for (int i = 0; i < 4; ++i)
    {
        hp[i].reset (sample_rate);
    }
}

double HighPassLadder::get_sample_rate() const
{
    return sample_rate;
}

//==============================================================================

void HighPassLadder::set_cutoff (const double cutoff)
{
    const double omega_digital = TWO_PI * cutoff;
    const double omega_analog = prewarp (omega_digital, sample_rate);

    g = omega_analog / (2.0 * sample_rate);
    G = 1.0 / (1.0 + g);
    G2 = G * G;
    G3 = G2 * G;
    G4 = G3 * G;

    for (int i = 0; i < 4; ++i)
    {
        hp[i].set_G (G);
        hp[i].set_g2 (g + g);
    }
}

void HighPassLadder::set_resonance (const double resonance)
{
    k = map_linear_normalized (resonance, 0.0, 4.0);
}

//==============================================================================

double HighPassLadder::process (double x)
{
    const double s1 = hp[0].get_state();
    const double s2 = hp[1].get_state();
    const double s3 = hp[2].get_state();
    const double s4 = hp[3].get_state();

    const double S = -G4 * s1 - G3 * s2 - G2 * s3 - G * s4;
    double y = (x - k * S) / (1.0 + k * G4);

    for (int i = 0; i < 4; ++i)
    {
        y = hp[i].process (y);
    }

    return y * (1.0 + k);
}