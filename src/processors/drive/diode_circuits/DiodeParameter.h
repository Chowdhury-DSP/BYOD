#pragma once

#include "../../ParameterHelpers.h"

namespace DiodeParameter
{
using namespace ParameterHelpers;

inline float getDiodeIs (int diodeType)
{
    switch (diodeType)
    {
        case 0: // GZ34
            return 2.52e-9f;
        case 1: // 1N34
            return 200.0e-12f;
        case 2: // 1N4148
            return 2.64e-9f;
        default:
            break;
    }

    jassertfalse;
    return 1.0e-9f;
}

inline void createDiodeParam (Params& params, const String& id)
{
    params.push_back (std::make_unique<AudioParameterChoice> (id,
                                                              "Diodes",
                                                              StringArray { "GZ34", "1N34", "1N4148" },
                                                              0));
}

inline void createNDiodesParam (Params& params, const String& id)
{
    auto nDiodesRange = createNormalisableRange (0.3f, 3.0f, 1.0f);

    params.push_back (std::make_unique<VTSParam> (id,
                                                  "# Diodes",
                                                  String(),
                                                  nDiodesRange,
                                                  1.0f,
                                                  &floatValToString,
                                                  &stringToFloatVal));
}

} // namespace DiodeParameter
