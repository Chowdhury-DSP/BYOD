#pragma once

// One-pole low-pass filter modeled by the TPT approach.
// Used in the transistor-ladder filter cascade.
// (See Zavalishin "The Art of VA Filter Design")
class LowPassOnePole
{
public:
    LowPassOnePole() = default;
    ~LowPassOnePole() = default;

    // Initialize filter
    void reset (const double _sample_rate);

    double get_sample_rate() const;

    //==============================================================================

    // Set "instantaneous gain" of the filter
    // Indirectly sets the filter cutoff frequency
    // (G is calculated when setting the cutoff of the full ladder structure)
    void set_G (const double _g);

    // Get current contents of the internal state register
    double get_state() const;

    //==============================================================================

    // Process a single audio sample
    inline double process (double x)
    {
        // Calculate output sample
        const double v = G * (x - state);
        const double y = v + state;

        // Update state register
        state = v + y;

        return y;
    }

private:
    double sample_rate { 0.0 };
    double state { 0.0 }; // Internal state register

    double G { 0.0 }; // Filter "instantaneous gain"
};