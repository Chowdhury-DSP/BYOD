/*
** Surge Synthesizer is Free and Open Source Software
**
** Surge is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Surge was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Surge
** open source in September 2018.
*/

#pragma once

#include <pch.h>

/**
 * This code contains a bunch of waveshaper code that was borrowed from the Surge Synthesizer waveshaper effect.
 * Majority of the waveshapers here were written by Paul (@baconpaul), and are borrowed with his permission.
 *
 * wst_soft, wst_hard, wst_asym, wst_digital, and wst_sine have been completely re-written, so there's no issue there.
 *
 * Waiting to hear back from the original developer if we can use wst_ojd...
 */
namespace SurgeWaveshapers
{
using Vec4 = dsp::SIMDRegister<float>;

enum ws_type
{
    wst_soft,
    wst_hard,
    wst_asym,
    wst_sine,
    wst_digital,

    // XT waves start here
    wst_cheby2,
    wst_cheby3,
    wst_cheby4,
    wst_cheby5,

    wst_fwrectify,
    wst_poswav,
    wst_negwav,
    wst_softrect,

    wst_singlefold,
    wst_dualfold,
    wst_westfold,

    // additive harmonics
    wst_add12,
    wst_add13,
    wst_add14,
    wst_add15,
    wst_add12345,
    wst_addsaw3,
    wst_addsqr3,

    wst_fuzz,
    wst_fuzzsoft,
    wst_fuzzheavy,
    wst_fuzzctr,
    wst_fuzzsoftedge,

    wst_sinpx,
    wst_sin2xpb,
    wst_sin3xpb,
    wst_sin7xpb,
    wst_sin10xpb,

    wst_2cyc,
    wst_7cyc,
    wst_10cyc,

    wst_2cycbound,
    wst_7cycbound,
    wst_10cycbound,

    wst_zamsat,
    wst_ojd,

    wst_softfold,

    n_ws_types,
};

extern StringArray wst_names;

constexpr int n_waveshaper_registers = 4;

struct alignas (16) QuadFilterWaveshaperState
{
    Vec4 R[n_waveshaper_registers];
    dsp::SIMDRegister<uint32_t> init;
};
typedef Vec4 (*WaveshaperQFPtr) (QuadFilterWaveshaperState* __restrict, Vec4 in, Vec4 drive);
WaveshaperQFPtr GetQFPtrWaveshaper (int type);

/*
 * Given the very first sample inbound to a new voice session, return the
 * first set of registers for that voice.
 */
void initializeWaveshaperRegister (int type, float R[n_waveshaper_registers]);
} // namespace SurgeWaveshapers
