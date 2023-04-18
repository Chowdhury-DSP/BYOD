#include "LadderParameters.h"

LadderParameters::LadderParameters (juce::AudioProcessorValueTreeState& _vts) : vts (_vts)
{
    using namespace ParameterHelpers;

    loadParameterPointer (drive_norm, vts, "DRIVE");

    loadParameterPointer (hp_cutoff_norm, vts, "HP_CUTOFF");
    loadParameterPointer (hp_resonance_norm, vts, "HP_RESONANCE");

    loadParameterPointer (lp_cutoff_norm, vts, "LP_CUTOFF");
    loadParameterPointer (lp_resonance_norm, vts, "LP_RESONANCE");

    filter_mode_norm = vts.getRawParameterValue("FILTER_MODE");
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

    emplace_param<AudioParameterChoice> (params, "FILTER_MODE", "FILTER MODE", StringArray { "Normal", "Oscillating" }, 0);

    return { params.begin(), params.end() };
}