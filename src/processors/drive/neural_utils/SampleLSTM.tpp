#include "SampleLSTM.h"

template <typename T, int in_sizet, int out_sizet>
SampleLSTM<T, in_sizet, out_sizet>::SampleLSTM()
{
    for (int i = 0; i < v_out_size; ++i)
    {
        // single-input kernel weights
        Wf_1[i] = v_type ((T) 0);
        Wi_1[i] = v_type ((T) 0);
        Wo_1[i] = v_type ((T) 0);
        Wc_1[i] = v_type ((T) 0);

        // biases
        bf[i] = v_type ((T) 0);
        bi[i] = v_type ((T) 0);
        bo[i] = v_type ((T) 0);
        bc[i] = v_type ((T) 0);

        // intermediate vars
        ft[i] = v_type ((T) 0);
        it[i] = v_type ((T) 0);
        ot[i] = v_type ((T) 0);
        ht[i] = v_type ((T) 0);
    }

    for (int i = 0; i < out_size; ++i)
    {
        // recurrent weights
        for (int k = 0; k < v_out_size; ++k)
        {
            Uf[i][k] = v_type ((T) 0);
            Ui[i][k] = v_type ((T) 0);
            Uo[i][k] = v_type ((T) 0);
            Uc[i][k] = v_type ((T) 0);
        }

        // kernel weights
        for (int k = 0; k < v_in_size; ++k)
        {
            Wf[i][k] = v_type ((T) 0);
            Wi[i][k] = v_type ((T) 0);
            Wo[i][k] = v_type ((T) 0);
            Wc[i][k] = v_type ((T) 0);
        }
    }
}

template <typename T, int in_sizet, int out_sizet>
void SampleLSTM<T, in_sizet, out_sizet>::prepare (int delaySamples)
{
    delayWriteIdx = delaySamples - 1;
    ct_internal.resize (delayWriteIdx + 1, {});
    outs_internal.resize (delayWriteIdx + 1, {});

    reset();
}

template <typename T, int in_sizet, int out_sizet>
void SampleLSTM<T, in_sizet, out_sizet>::reset()
{
    for (auto& vec : ct_internal)
        std::fill (vec.begin(), vec.end(), v_type{});

    for (auto& vec : outs_internal)
        std::fill (vec.begin(), vec.end(), v_type{});

    // reset output state
    for (int i = 0; i < v_out_size; ++i)
    {
        ct[i] = v_type ((T) 0);
        outs[i] = v_type ((T) 0);
    }
}

template <typename T, int in_sizet, int out_sizet>
void SampleLSTM<T, in_sizet, out_sizet>::setWVals (const std::vector<std::vector<T>>& wVals)
{
    for (int i = 0; i < in_size; ++i)
    {
        for (int j = 0; j < out_size; ++j)
        {
            Wi[j][i / v_size] = RTNeural::set_value (Wi[j][i / v_size], i % v_size, wVals[i][j]);
            Wf[j][i / v_size] = RTNeural::set_value (Wf[j][i / v_size], i % v_size, wVals[i][j + out_size]);
            Wc[j][i / v_size] = RTNeural::set_value (Wc[j][i / v_size], i % v_size, wVals[i][j + 2 * out_size]);
            Wo[j][i / v_size] = RTNeural::set_value (Wo[j][i / v_size], i % v_size, wVals[i][j + 3 * out_size]);
        }
    }

    for (int j = 0; j < out_size; ++j)
    {
        Wi_1[j / v_size] = RTNeural::set_value (Wi_1[j / v_size], j % v_size, wVals[0][j]);
        Wf_1[j / v_size] = RTNeural::set_value (Wf_1[j / v_size], j % v_size, wVals[0][j + out_size]);
        Wc_1[j / v_size] = RTNeural::set_value (Wc_1[j / v_size], j % v_size, wVals[0][j + 2 * out_size]);
        Wo_1[j / v_size] = RTNeural::set_value (Wo_1[j / v_size], j % v_size, wVals[0][j + 3 * out_size]);
    }
}

template <typename T, int in_sizet, int out_sizet>
void SampleLSTM<T, in_sizet, out_sizet>::setUVals (const std::vector<std::vector<T>>& uVals)
{
    for (int i = 0; i < out_size; ++i)
    {
        for (int j = 0; j < out_size; ++j)
        {
            Ui[j][i / v_size] = RTNeural::set_value (Ui[j][i / v_size], i % v_size, uVals[i][j]);
            Uf[j][i / v_size] = RTNeural::set_value (Uf[j][i / v_size], i % v_size, uVals[i][j + out_size]);
            Uc[j][i / v_size] = RTNeural::set_value (Uc[j][i / v_size], i % v_size, uVals[i][j + 2 * out_size]);
            Uo[j][i / v_size] = RTNeural::set_value (Uo[j][i / v_size], i % v_size, uVals[i][j + 3 * out_size]);
        }
    }
}

template <typename T, int in_sizet, int out_sizet>
void SampleLSTM<T, in_sizet, out_sizet>::setBVals (const std::vector<T>& bVals)
{
    for (int k = 0; k < out_size; ++k)
    {
        bi[k / v_size] = RTNeural::set_value (bi[k / v_size], k % v_size, bVals[k]);
        bf[k / v_size] = RTNeural::set_value (bf[k / v_size], k % v_size, bVals[k + out_size]);
        bc[k / v_size] = RTNeural::set_value (bc[k / v_size], k % v_size, bVals[k + 2 * out_size]);
        bo[k / v_size] = RTNeural::set_value (bo[k / v_size], k % v_size, bVals[k + 3 * out_size]);
    }
}
