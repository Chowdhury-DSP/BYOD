#pragma once

#include "../../ParameterHelpers.h"
#include <JuceHeader.h>

#include "VoltPerOct.h"
#include "utility.h"

// This class handles processor parameters used in the dsp calculations
// Internally it uses a virtual "volt per octave" standard for the filter responses
class LadderParameters
{
public:
    LadderParameters (juce::AudioProcessorValueTreeState& _vts);
    ~LadderParameters() = default;

    //==============================================================================

    // Initialize parameter smoothing
    void reset (const double sample_rate, const int samples_per_block);

    // Create parameter layout
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // Methods used to access parameter values for audio processing
    // Parameter denormalization is handled here

    double drive();
    double drive_normalized();

    double lp_cutoff();
    double lp_resonance() const;

    double hp_cutoff();
    double hp_resonance() const;

private:
    juce::AudioProcessorValueTreeState& vts;

    //==============================================================================
    // Atomic pointers to access raw (normalized) parameter values stored in the VTS

    std::atomic<float>* drive_norm;

    std::atomic<float>* hp_cutoff_norm;
    std::atomic<float>* hp_resonance_norm;

    std::atomic<float>* lp_cutoff_norm;
    std::atomic<float>* lp_resonance_norm;

    //==============================================================================
    // Filters to smooth out parameter changes

    ParameterSmoother drive_smooth;

    ParameterSmoother lp_cutoff_smooth;
    ParameterSmoother hp_cutoff_smooth;

    //==============================================================================
    // Virtual volt per octave for the filters
    VoltPerOct filter_vpo;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LadderParameters)
};