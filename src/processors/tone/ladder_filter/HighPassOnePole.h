#pragma once

// One-pole high-pass filter modeled by the TPT approach.
// Used in the transistor-ladder filter cascade.
// (See Zavalishin "The Art of VA Filter Design")
class HighPassOnePole
{
public:
    HighPassOnePole() = default;
    ~HighPassOnePole() = default;

    // Initialize filter
    void reset (const double _sample_rate);

    double get_sample_rate() const;

    //==============================================================================

    // Set the "instantaneous gain" of the filter
    // Indirectly sets the filter cutoff frequency
    // (G is calculated when setting the cutoff of the full ladder structure)
    void set_G (const double _G);

    // Set the "instantaneous gain" of the trapezoidal integrator times two
    // (g2 is calculated when setting the cutoff of the full ladder structure)
    void set_g2 (const double _g2);

    // Get current contents of the internal state register
    double get_state() const;

    //==============================================================================

    // Process a single audio sample
    inline double process (double x)
    {
        // Calculate output sample
        const double y = G * (x - state);

        // Update state register
        state = state + y * g2;

        return y;
    }

private:
    double sample_rate { 0.0 };
    double state { 0.0 }; // State register

    double G { 0.0 }; // Filter "instantaneous gain"
    double g2 { 0.0 }; // TPT integrator "instantaneous gain" times two
};