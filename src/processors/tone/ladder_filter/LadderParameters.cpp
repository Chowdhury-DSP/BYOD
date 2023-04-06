#include "LadderParameters.h"

LadderParameters::LadderParameters (juce::AudioProcessorValueTreeState& _vts) : vts (_vts)
{
    // Get nomralized parameter values from the Apvts

    drive_norm = vts.getRawParameterValue ("DRIVE");

    hp_cutoff_norm = vts.getRawParameterValue ("HP_CUTOFF");
    hp_resonance_norm = vts.getRawParameterValue ("HP_RESONANCE");

    lp_cutoff_norm = vts.getRawParameterValue ("LP_CUTOFF");
    lp_resonance_norm = vts.getRawParameterValue ("LP_RESONANCE");

    // Tune volt/octave conversions, so that -5V to +5V spans 10 octaves above MIN_FILTER_FREQ
    filter_vpo.set_zero_volt_freq (ladder_filter_utility::MIN_FILTER_FREQ * pow (2.0, 5.0));
}

//==============================================================================

void LadderParameters::reset (const double sample_rate, const int samples_per_block)
{
    // Set cutoff smoothing time constants to 10 ms
    drive_smooth.reset (0.8f, static_cast<float> (sample_rate), samples_per_block);
    lp_cutoff_smooth.reset (0.8f, static_cast<float> (sample_rate), samples_per_block);
    hp_cutoff_smooth.reset (0.8f, static_cast<float> (sample_rate), samples_per_block);
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

    return { params.begin(), params.end() };
}

//==============================================================================

double LadderParameters::drive()
{
    const double decibels = ladder_filter_utility::map_linear_normalized (static_cast<double> (*drive_norm), -24.0, 24.0);
    double gain_factor = ladder_filter_utility::decibel_to_raw_gain (decibels);

    return drive_smooth.process (gain_factor);
}

double LadderParameters::drive_normalized()
{
    return static_cast<double> (*drive_norm);
}

double LadderParameters::lp_cutoff()
{
    const double control_voltage = ladder_filter_utility::map_linear_normalized (static_cast<double> (*lp_cutoff_norm), -5.0, 5.0);
    double cutoff = filter_vpo.volt_to_freq (control_voltage);

    return lp_cutoff_smooth.process (cutoff);
}

double LadderParameters::lp_resonance() const
{
    return ladder_filter_utility::skew_normalized (static_cast<double> (*lp_resonance_norm), 0.33);
}

double LadderParameters::hp_cutoff()
{
    const double control_voltage = ladder_filter_utility::map_linear_normalized (static_cast<double> (*hp_cutoff_norm), -5.0, 5.0);
    double cutoff = filter_vpo.volt_to_freq (control_voltage);

    return hp_cutoff_smooth.process (cutoff);
}

double LadderParameters::hp_resonance() const
{
    return ladder_filter_utility::skew_normalized (static_cast<double> (*hp_resonance_norm), 0.33);
}