#pragma once

#include <pch.h>

namespace ParameterHelpers
{
using namespace chowdsp::ParamUtils;
using Params = chowdsp::Parameters;

inline auto createBaseParams()
{
    Params params;
    emplace_param<AudioParameterBool> (params, "on_off", "On/Off", true);

    return std::move (params);
}

inline float logPot (float x)
{
    return (std::pow (10.0f, x) - 1.0f) / 9.0f;
}

inline float iLogPot (float x)
{
    return (std::pow (0.1f, x) - 1.0f) / -0.9f;
}
} // namespace ParameterHelpers
