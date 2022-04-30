#pragma once

#include <pch.h>

/**
 * Modification of RTNeural::LSTMLayerT to use a variable recurrent delay
 * instead of the standard 1-sample recurrent delay.
 *
 * N.B: Right now this calss is specialized for in_sizet = 1.
 */
template <typename T, int in_sizet, int out_sizet>
class SampleGRU
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_in_size = RTNeural::ceil_div(in_sizet, v_size);
    static constexpr auto v_out_size = RTNeural::ceil_div(out_sizet, v_size);

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;

    static_assert (in_size == 1, "SampleGRU is currently only implemented for 1-input case!");

    SampleGRU();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "gru"; }

    /** Returns false since GRU is not an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Prepares the LSTM to process with a given delay length. */
    void prepare (int delaySamples);

    /** Resets the state of the GRU. */
    void reset();

    /** Performs forward propagation for this layer. */
    void forward(const v_type (&ins)[v_in_size])
    {
        // compute zt
        recurrent_mat_mul(outs, Uz, zt);
        for(int i = 0; i < v_out_size; ++i)
            zt[i] = sigmoid(xsimd::fma(Wz_1[i], ins[0], zt[i] + bz[i]));

        // compute rt
        recurrent_mat_mul(outs, Ur, rt);
        for(int i = 0; i < v_out_size; ++i)
            rt[i] = sigmoid(xsimd::fma(Wr_1[i], ins[0], rt[i] + br[i]));

        // compute h_hat
        recurrent_mat_mul(outs, Uh, ct);
        for(int i = 0; i < v_out_size; ++i)
            ht[i] = xsimd::tanh(xsimd::fma(rt[i], ct[i] + bh1[i], xsimd::fma(Wh_1[i], ins[0], bh0[i])));

        // compute output
        for(int i = 0; i < v_out_size; ++i)
            outs_internal[delayWriteIdx][i] = xsimd::fma((v_type((T)1.0) - zt[i]), ht[i], zt[i] * outs[i]);

        processDelay (outs_internal, outs, delayWriteIdx);
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][3 * out_size]
     */
    void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][3 * out_size]
     */
    void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[2][3 * out_size]
     */
    void setBVals(const std::vector<std::vector<T>>& bVals);

    v_type outs[v_out_size];

private:
    static void processDelay (std::vector<std::array<v_type, v_out_size>>& delayVec, v_type (&out)[v_out_size], int delayWriteIndex)
    {
        for (int i = 0; i < v_out_size; ++i)
            out[i] = delayVec[0][i];

        for (int j = 0; j < delayWriteIndex; ++j)
        {
            for (int i = 0; i < v_out_size; ++i)
                delayVec[j][i] = delayVec[j + 1][i];
        }
    }

    static inline void recurrent_mat_mul(const v_type (&vec)[v_out_size], const v_type (&mat)[out_size][v_out_size], v_type (&out)[v_out_size]) noexcept
    {
        for(int i = 0; i < v_out_size; ++i)
            out[i] = v_type(0);

        T scalar_in alignas(RTNEURAL_DEFAULT_ALIGNMENT)[v_size] { (T)0 };
        for(int k = 0; k < v_out_size; ++k)
        {
            vec[k].store_aligned(scalar_in);
            for(int i = 0; i < v_out_size; ++i)
            {
                for(int j = 0; j < v_size; ++j)
                    out[i] += scalar_in[j] * mat[k * v_size + j][i];
            }
        }
    }

    static inline v_type sigmoid(v_type x) noexcept
    {
        return (T)1.0 / ((T)1.0 + xsimd::exp(-x));
    }

    // single-input kernel weights
    v_type Wz_1[v_out_size];
    v_type Wr_1[v_out_size];
    v_type Wh_1[v_out_size];

    // recurrent weights
    v_type Uz[out_size][v_out_size];
    v_type Ur[out_size][v_out_size];
    v_type Uh[out_size][v_out_size];

    // biases
    v_type bz[v_out_size];
    v_type br[v_out_size];
    v_type bh0[v_out_size];
    v_type bh1[v_out_size];

    // intermediate vars
    v_type zt[v_out_size];
    v_type rt[v_out_size];
    v_type ct[v_out_size];
    v_type ht[v_out_size];

    std::vector<std::array<v_type, v_out_size>> outs_internal;
    int delayWriteIdx = 0;
};

#include "SampleGRU.tpp"
