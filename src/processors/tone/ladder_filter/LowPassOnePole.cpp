#include "LowPassOnePole.h"

void LowPassOnePole::reset (const double _sample_rate)
{
    sample_rate = _sample_rate;
    state = 0.0;
}

double LowPassOnePole::get_sample_rate() const
{
    return sample_rate;
}

//==============================================================================

void LowPassOnePole::set_G (const double _G)
{
    G = _G;
}

double LowPassOnePole::get_state() const
{
    return state;
}

//==============================================================================

double LowPassOnePole::process (double x)
{
    // Calculate output sample
    const double v = G * (x - state);
    const double y = v + state;

    // Update state register
    state = v + y;

    return y;
}