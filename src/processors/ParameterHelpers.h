#pragma once

#include <pch.h>

namespace ParameterHelpers
{
using namespace chowdsp::ParamUtils;
using Params = std::vector<std::unique_ptr<RangedAudioParameter>>;

inline void createFreqParameter (Params& params, const String& id, const String& name, float min, float max, float centre, float defaultValue)
{
    NormalisableRange<float> freqRange { min, max };
    freqRange.setSkewForCentre (centre);

    params.push_back (std::make_unique<VTSParam> (id,
                                                  name,
                                                  String(),
                                                  freqRange,
                                                  defaultValue,
                                                  &freqValToString,
                                                  &stringToFreqVal));
}

inline void createPercentParameter (Params& params, const String& id, const String& name, float defaultValue)
{
    params.push_back (std::make_unique<VTSParam> (id,
                                                  name,
                                                  String(),
                                                  NormalisableRange<float> { 0.0f, 1.0f },
                                                  defaultValue,
                                                  &percentValToString,
                                                  &stringToPercentVal));
}

inline Params createBaseParams()
{
    Params params;
    params.push_back (std::make_unique<AudioParameterBool> ("on_off", "On/Off", true));

    return std::move (params);
}

static float logPot (float x)
{
    return (std::pow (10.0f, x) - 1.0f) / 9.0f;
}

static float iLogPot (float x)
{
    return (std::pow (0.1f, x) - 1.0f) / -0.9f;
}

}; // namespace ParameterHelpers
