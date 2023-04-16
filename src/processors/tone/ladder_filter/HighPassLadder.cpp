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
    const double omega_digital = ladder_filter_utility::TWO_PI * cutoff;
    const double omega_analog = ladder_filter_utility::prewarp (omega_digital, sample_rate);

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
    k = resonance;
}