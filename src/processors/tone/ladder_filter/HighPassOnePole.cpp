#include "HighPassOnePole.h"

void HighPassOnePole::reset (const double _sample_rate)
{
    sample_rate = _sample_rate;
    state = 0.0;
}

double HighPassOnePole::get_sample_rate() const
{
    return sample_rate;
}

//==============================================================================

void HighPassOnePole::set_G (const double _G)
{
    G = _G;
}

void HighPassOnePole::set_g2 (const double _g2)
{
    g2 = _g2;
}

double HighPassOnePole::get_state() const
{
    return state;
}

//==============================================================================

double HighPassOnePole::process (double x)
{
    // Calculate output sample
    const double y = G * (x - state);

    // Update state register
    state = state + y * g2;

    return y;
}