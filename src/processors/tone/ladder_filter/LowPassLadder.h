#pragma once

#include "LowPassOnePole.h"
#include "utility.h"

// Four-pole resonant low-pass filter modeled after the famous transistor-ladder.
// Model was created using the TPT approach.
// Capable of self-oscillation, strictly linear model
// (See Zavalishin "The Art of VA Filter Design")
class LowPassLadder
{
public:
    LowPassLadder() = default;
    ~LowPassLadder() = default;

    // Initialize filter
    void reset (const double _sample_rate);

    double get_sample_rate() const;

    //==============================================================================

    // Set ladder filter cutoff frequency [Hz]
    // Range: 20 Hz to 20480 Hz (10 octaves, approximate range of human hearing)
    void set_cutoff (const double cutoff);

    // Set ladder filter resonance [1]
    // Range: 0.0 to 1.0
    void set_resonance (const double resonance);

    //==============================================================================

    // Process a single audio sample
    inline double process (double x)
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

private:
    double sample_rate { 0.0 };

    double k { 0.0 }; // Feedback gain

    double g { 0.0 }; // Trapezoidal integrator "instantaneous gain"
    double G { 0.0 }; // One-pole filter "instantaneous gain"
    double G2 { 0.0 }; // G * G
    double G3 { 0.0 }; // G * G * G
    double G4 { 0.0 }; // G * G * G * G

    // Chain of one-pole filters, as given by the classic ladder structure
    LowPassOnePole lp[4];
};