#pragma once

#include <cmath>

namespace ladder_filter_utility
{

//==============================================================================
// Useful constants

constexpr double PI =
    3.14159265358979323846264338327950288419716939937510582097494459230781640628620899;

constexpr double TWO_PI =
    2.0 * 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899;

// Minimum filter cutoff frequency: 20 Hz
constexpr double MIN_FILTER_FREQ = 20.0;

// Maximum filter cutoff frequency: 20480 Hz (10 octaves above 20 Hz)
constexpr double MAX_FILTER_FREQ = 20480.0;

constexpr double ZERO_VOLT_FREQ = MIN_FILTER_FREQ * 2.0 * 2.0 * 2.0 * 2.0 * 2.0;

//==============================================================================
// Mappings / skews

// Linearly map a normalized (unit interval) value to a given range
template <typename T = double>
inline T map_linear_normalized (const T val, const T out_min, const T out_max)
{
    return val * (out_max - out_min) + out_min;
}

// Prewarp a frequency to be preserved in the topology-preserving transformation
inline double prewarp (const double omega_digital, const double sample_rate)
{
    const double omega_analog = 2.0 * sample_rate * tan (omega_digital * 1.0 / (2.0 * sample_rate));
    return omega_analog;
}

// Convert decibel value to a raw gain factor
inline double decibel_to_raw_gain (double decibel)
{
    return pow (10.0, decibel / 20.0);
}

// Skew a normalized (unit interval) value.
// skew = 1 results in a linear response
// skew in ]0.0, 1.0[ gives more resolution in the lower range
// skew in ]1.0, INF[ gives more resolution in the upper range
inline double skew_normalized (const double normalized_val, const double skew)
{
    return pow (normalized_val, 1.0 / skew);
}

// Limit value to not exceed an upper (positive) threshold
inline double limit_upper (const double value_to_limit, const double limit)
{
    if (value_to_limit >= limit)
    {
        return limit;
    }
    else
    {
        return value_to_limit;
    }
}

// Convert control-voltage to requency
inline double volt_to_freq (const double& volt)
{
    return ZERO_VOLT_FREQ * pow (2.0, volt);
}

//==============================================================================
// Static waveshaper

// Faster hyperbolic tangent approximation
//
// - 0.427% peak relative error
// - Smooth 1st and 2nd derivatives
// - 3rd derivative's magnitude relative to the 1st derivative at discontinuity point is around -191 dB
//
// As proposed by Aleksey Vaneef:
// https://www.kvraudio.com/forum/viewtopic.php?f=33&t=388650&start=45
template <typename T>
inline T fast_tanh_2 (T x)
{
    const auto ax = fabs (x);
    const auto x2 = x * x;

    return (x * ((T) 2.45550750702956 + (T) 2.45550750702956 * ax + ((T) 0.893229853513558 + (T) 0.821226666969744 * ax) * x2) / ((T) 2.44506634652299 + ((T) 2.44506634652299 + x2) * fabs (x + (T) 0.814642734961073 * x * ax)));
}
} // namespace ladder_filter_utility