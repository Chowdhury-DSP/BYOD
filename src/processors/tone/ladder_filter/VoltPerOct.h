#pragma once

#include <cmath>

// Virtual V/Oct as commonly found in Eurorack synthesizers.
// Used to model parameter and modulation repsonses of audio effect circuitry.
class VoltPerOct
{
public:
    VoltPerOct() = default;
    ~VoltPerOct() = default;

    //==============================================================================

    // Tune the conversion
    void set_zero_volt_freq (const double _zero_volt_freq)
    {
        zero_volt_freq = _zero_volt_freq;
    }

    // Convert control-voltage to requency
    inline double volt_to_freq (const double& volt) const
    {
        return zero_volt_freq * pow (2.0, volt);
    }

protected:
    double zero_volt_freq { 0.0f }; // Tuning (frequency corresponding to zero volt)
};