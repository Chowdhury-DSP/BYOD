#pragma once

#include <pch.h>

struct OmegaProvider
{
    template <typename T>
    static T omega (T x)
    {
        return math_approx::wright_omega<3, 3> (x);
    }
};
