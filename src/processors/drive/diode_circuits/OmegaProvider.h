#pragma once

#include <pch.h>

#if BYOD_USE_MATH_APPROX
struct OmegaProvider
{
    template <typename T>
    static T omega (T x)
    {
        return math_approx::wright_omega<3, 3> (x);
    }
};
#else
using OmegaProvider = chowdsp::Omega::Omega;
#endif
