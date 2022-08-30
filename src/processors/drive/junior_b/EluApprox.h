#pragma once

#include <pch.h>

/** Static implementation of a elu activation layer, using an approximation of the exp() function. */
template <typename T, int size>
class ELuActivationApproxT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int) v_type::size;
    static constexpr auto v_io_size = RTNeural::ceil_div (size, v_size);

public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;

    ELuActivationApproxT() = default;

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "elu"; }

    /** Returns true since this layer is an activation layer. */
    constexpr bool isActivation() const noexcept { return true; }

    void reset() {}

    /** Performs forward propagation for elu activation. */
    inline void forward (const v_type (&ins)[v_io_size])
    {
        for (int i = 0; i < v_io_size; ++i)
            outs[i] = xsimd::select (ins[i] > (T) 0, ins[i], chowdsp::Omega::exp_approx (ins[i]) - (T) 1);
    }

    v_type outs[v_io_size];
};
