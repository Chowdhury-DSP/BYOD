#pragma once

#include <pch.h>

namespace ParameterHelpers
{
using namespace chowdsp::ParamUtils;
using Params = chowdsp::Parameters;

inline void createFreqParameter (Params& params, const String& id, const String& name, float min, float max, float centre, float defaultValue)
{
    NormalisableRange<float> freqRange { min, max };
    freqRange.setSkewForCentre (centre);

    emplace_param<VTSParam> (params, id, name, String(), freqRange, defaultValue, &freqValToString, &stringToFreqVal);
}

inline void createPercentParameter (Params& params, const String& id, const String& name, float defaultValue)
{
    NormalisableRange<float> range { 0.0f, 1.0f };
    emplace_param<VTSParam> (params, id, name, String(), range, defaultValue, &percentValToString, &stringToPercentVal);
}

inline void createGainDBParameter (Params& params, const String& id, const String& name, float min, float max, float defaultValue, float centerValue = -1000.0f)
{
    NormalisableRange<float> range { min, max };
    if (centerValue > -1000.0f)
        range.setSkewForCentre (centerValue);

    emplace_param<VTSParam> (params, id, name, String(), range, defaultValue, &gainValToString, &stringToGainVal);
}

inline auto createBaseParams()
{
    Params params;
    emplace_param<AudioParameterBool> (params, "on_off", "On/Off", true);

    return std::move (params);
}

static inline float logPot (float x)
{
    return (std::pow (10.0f, x) - 1.0f) / 9.0f;
}

static inline float iLogPot (float x)
{
    return (std::pow (0.1f, x) - 1.0f) / -0.9f;
}

}; // namespace ParameterHelpers
