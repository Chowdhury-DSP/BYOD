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

private:
    juce::AudioProcessorValueTreeState& vts;

public:
    // Limit of resonance setting to keep the filters from self-oscillating depending on filter mode
    static constexpr double RESONANCE_LIMIT_NON_OSCILLATING = 0.96;

    //==============================================================================
    // Pointers to access raw (normalized) parameter values stored in the VTS

    chowdsp::FloatParameter* drive_norm { nullptr };

    chowdsp::FloatParameter* hp_cutoff_norm { nullptr };
    chowdsp::FloatParameter* hp_resonance_norm { nullptr };

    chowdsp::FloatParameter* lp_cutoff_norm { nullptr };
    chowdsp::FloatParameter* lp_resonance_norm { nullptr };

    std::atomic<float>* filter_mode_norm { nullptr };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LadderParameters)
};