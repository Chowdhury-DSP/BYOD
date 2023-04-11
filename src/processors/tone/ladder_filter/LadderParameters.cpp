#include "LadderParameters.h"

LadderParameters::LadderParameters (juce::AudioProcessorValueTreeState& _vts) : vts (_vts)
{
    // Get nomralized parameter values from the Apvts

    drive_norm = vts.getRawParameterValue ("DRIVE");

    hp_cutoff_norm = vts.getRawParameterValue ("HP_CUTOFF");
    hp_resonance_norm = vts.getRawParameterValue ("HP_RESONANCE");

    lp_cutoff_norm = vts.getRawParameterValue ("LP_CUTOFF");
    lp_resonance_norm = vts.getRawParameterValue ("LP_RESONANCE");

    filter_mode_norm = vts.getRawParameterValue ("FILTER_MODE");
}

AudioProcessorValueTreeState::ParameterLayout LadderParameters::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "HP_CUTOFF", "HP CUT", 0.0f);
    createPercentParameter (params, "HP_RESONANCE", "HP RES", 0.0f);
    createPercentParameter (params, "LP_RESONANCE", "LP RES", 0.0f);
    createPercentParameter (params, "LP_CUTOFF", "LP CUT", 1.0f);
    createPercentParameter (params, "DRIVE", "DRIVE", 0.5f);

    // emplace_param<AudioParameterChoice> (params, modeTag, "Mode", StringArray { "Traditional", "Neural" }, 0);
    emplace_param<AudioParameterChoice> (params, "FILTER_MODE", "FILTER MODE", StringArray { "Normal", "Oscillating" }, 0);

    return { params.begin(), params.end() };
}