#pragma once

#include "HighPassOnePole.h"
#include "utility.h"

// Four-pole resonant high-pass filter modeled after the famous transistor-ladder.
// Model was created using the TPT approach.
// Capable of self-oscillation, strictly linear model
// (See Zavalishin "The Art of VA Filter Design")
class HighPassLadder
{
public:
    HighPassLadder() = default;
    ~HighPassLadder() = default;

    void reset (const double _sample_rate);
    double get_sample_rate() const;

    //==============================================================================

    // Set ladder filter cutoff frequency [Hz]
    // Range: 20 Hz to 20480 Hz (10 octaves, approximate range of human hearing)
    void set_cutoff (const double cutoff);

    // Set ladder filter resonance [1]
    // Range: 0.0 to 4.0
    void set_resonance (const double resonance);

    //==============================================================================

    // Process a single audio sample
    inline double process (double x)
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
    HighPassOnePole hp[4];
};