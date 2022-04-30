#include "SampleGRU.h"

template <typename T, int in_sizet, int out_sizet>
SampleGRU<T, in_sizet, out_sizet>::SampleGRU()
{
    for (int i = 0; i < v_out_size; ++i)
    {
        // single-input kernel weights
        Wz_1[i] = v_type ((T) 0);
        Wr_1[i] = v_type ((T) 0);
        Wh_1[i] = v_type ((T) 0);

        // biases
        bz[i] = v_type ((T) 0);
        br[i] = v_type ((T) 0);
        bh0[i] = v_type ((T) 0);
        bh1[i] = v_type ((T) 0);

        // intermediate vars
        zt[i] = v_type ((T) 0);
        rt[i] = v_type ((T) 0);
        ct[i] = v_type ((T) 0);
        ht[i] = v_type ((T) 0);
    }

    // recurrent weights
    for (int i = 0; i < out_size; ++i)
    {
        for (int k = 0; k < v_out_size; ++k)
        {
            Uz[i][k] = v_type ((T) 0);
            Ur[i][k] = v_type ((T) 0);
            Uh[i][k] = v_type ((T) 0);
        }
    }

    reset();
}

template <typename T, int in_sizet, int out_sizet>
void SampleGRU<T, in_sizet, out_sizet>::prepare (int delaySamples)
{
    delayWriteIdx = delaySamples - 1;
    outs_internal.resize (delayWriteIdx + 1, {});

    reset();
}

template <typename T, int in_sizet, int out_sizet>
void SampleGRU<T, in_sizet, out_sizet>::reset()
{
    for (auto& vec : outs_internal)
        std::fill (vec.begin(), vec.end(), v_type {});

    // reset output state
    for (int i = 0; i < v_out_size; ++i)
        outs[i] = v_type ((T) 0);
}

// kernel weights
template <typename T, int in_sizet, int out_sizet>
void SampleGRU<T, in_sizet, out_sizet>::setWVals (const std::vector<std::vector<T>>& wVals)
{
    for (int j = 0; j < out_size; ++j)
    {
        Wz_1[j / v_size] = RTNeural::set_value (Wz_1[j / v_size], j % v_size, wVals[0][j]);
        Wr_1[j / v_size] = RTNeural::set_value (Wr_1[j / v_size], j % v_size, wVals[0][j + out_size]);
        Wh_1[j / v_size] = RTNeural::set_value (Wh_1[j / v_size], j % v_size, wVals[0][j + 2 * out_size]);
    }
}

// recurrent weights
template <typename T, int in_sizet, int out_sizet>
void SampleGRU<T, in_sizet, out_sizet>::setUVals (const std::vector<std::vector<T>>& uVals)
{
    for (int i = 0; i < out_size; ++i)
    {
        for (int k = 0; k < out_size; ++k)
        {
            Uz[k][i / v_size] = RTNeural::set_value (Uz[k][i / v_size], i % v_size, uVals[k][i]);
            Ur[k][i / v_size] = RTNeural::set_value (Ur[k][i / v_size], i % v_size, uVals[k][i + out_size]);
            Uh[k][i / v_size] = RTNeural::set_value (Uh[k][i / v_size], i % v_size, uVals[k][i + 2 * out_size]);
        }
    }
}

// biases
template <typename T, int in_sizet, int out_sizet>
void SampleGRU<T, in_sizet, out_sizet>::setBVals (const std::vector<std::vector<T>>& bVals)
{
    for (int k = 0; k < out_size; ++k)
    {
        bz[k / v_size] = RTNeural::set_value (bz[k / v_size], k % v_size, bVals[0][k] + bVals[1][k]);
        br[k / v_size] = RTNeural::set_value (br[k / v_size], k % v_size, bVals[0][k + out_size] + bVals[1][k + out_size]);
        bh0[k / v_size] = RTNeural::set_value (bh0[k / v_size], k % v_size, bVals[0][k + 2 * out_size]);
        bh1[k / v_size] = RTNeural::set_value (bh1[k / v_size], k % v_size, bVals[1][k + 2 * out_size]);
    }
}
