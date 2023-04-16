#pragma once

#include "../../ParameterHelpers.h"
#include <JuceHeader.h>

#include "utility.h"

// This class handles processor parameters used in the dsp calculations
// Internally it uses a virtual "volt per octave" standard for the filter responses
class LadderParameters
{
public:
    LadderParameters (juce::AudioProcessorValueTreeState& _vts);
    ~LadderParameters() = default;

    // Create parameter layout
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // Parameter denormalization functions / skews

    static inline float drive (float val)
    {
        const double decibels = ladder_filter_utility::map_linear_normalized (static_cast<double> (val), -24.0, 24.0);
        const double gain_factor = ladder_filter_utility::decibel_to_raw_gain (decibels);

        return gain_factor;
    }

    static inline float lp_cutoff (float val)
    {
        const double control_voltage = ladder_filter_utility::map_linear_normalized (static_cast<double> (val), -5.0, 5.0);
        const double cutoff = ladder_filter_utility::volt_to_freq (control_voltage);

        return cutoff;
    }

    static inline float lp_resonance (float val)
    {
        double resonance = ladder_filter_utility::skew_normalized (static_cast<double> (val), 0.33);

        return resonance;
    }

    static inline float hp_cutoff (float val)
    {
        const double control_voltage = ladder_filter_utility::map_linear_normalized (static_cast<double> (val), -5.0, 5.0);
        const double cutoff = ladder_filter_utility::volt_to_freq (control_voltage);

        return cutoff;
    }

    static inline float hp_resonance (float val)
    {
        double resonance = ladder_filter_utility::skew_normalized (static_cast<double> (val), 0.33);

        return resonance;
    }

private:
    juce::AudioProcessorValueTreeState& vts;

public:
    //==============================================================================
    // Pointers to access raw (normalized) parameter values stored in the VTS

    chowdsp::FloatParameter* drive_norm;

    chowdsp::FloatParameter* hp_cutoff_norm;
    chowdsp::FloatParameter* hp_resonance_norm;

    chowdsp::FloatParameter* lp_cutoff_norm;
    chowdsp::FloatParameter* lp_resonance_norm;

    chowdsp::FloatParameter* filter_mode_norm;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LadderParameters)
};