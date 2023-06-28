#include "FuzzFaceSolver.h"

namespace
{
constexpr auto vt = 0.026 * 0.5;
constexpr auto Iq = 1.0e-10;
} // namespace

void FuzzFaceSolver::prepare (double sample_rate)
{
    fs = sample_rate;
    gc1 = 2.0 * fs * C1;
    gc2 = 2.0 * fs * C2;

    reset();
}

void FuzzFaceSolver::reset()
{
    std::fill (ic1eq.begin(), ic1eq.end(), 0.0);
    std::fill (ic2eq.begin(), ic2eq.end(), 0.0);
    std::fill (vo.begin(), vo.end(), 0.0);
    std::fill (io.begin(), io.end(), 0.0);
}

void FuzzFaceSolver::set_gain (double gain_01) noexcept
{
    const auto gain_factor = chowdsp::Power::ipow<4> (gain_01 * 0.98 + 0.01);
    grfp = 1.0 / ((1.0 - gain_factor) * Rf);
    grfm = 1.0 / (gain_factor * Rf);
}

void FuzzFaceSolver::process (std::span<float> data, size_t ch) noexcept
{
    for (auto& sample : data)
    {
        const auto vi = std::tanh (0.1 * (double) sample);

        // initial guess
        auto v1 = -((-gr4 * (grfp * ic2eq[ch] + (gc2 + grfm + grfp) * io[ch]) + (gr4 * (gc2 + grfm) + (gc2 + gr4 + grfm) * grfp) * (ic1eq[ch] + io[ch] - gc1 * vi)) / (gc1 * gr4 * (gc2 + grfm) + gr4 * (gc2 + grfm) * grfp + gc1 * (gc2 + gr4 + grfm) * grfp));

        // Newton-Raphson Solver
        double delta;
        int n_iters = 0;
        auto v1_exp = std::exp (-v1 / vt);
        do
        {
            io[ch] = Iq * v1_exp;
            vo[ch] = (ic1eq[ch] + io[ch] + (gc1 + gr4) * v1 - gc1 * vi) / gr4;

            const auto F = (gc1 * (vi - v1) - ic1eq[ch]) + gr4 * (vo[ch] - v1) - io[ch];
            const auto F_p = -gc1 - gr4 + (Iq / vt) * v1_exp;
            delta = F / F_p;
            v1 -= delta;

            v1_exp = std::exp (-v1 / vt);
            n_iters++;
        } while (std::abs (delta) > 1.0e-5 && n_iters < 5);

        // compute output
        io[ch] = Iq * v1_exp;
        vo[ch] = (ic1eq[ch] + io[ch] + (gc1 + gr4) * v1 - gc1 * vi) / gr4;

        //update state
        const auto v2 = -((io[ch] + gr4 * v1 - (gr4 + grfp) * vo[ch]) / grfp);
        ic1eq[ch] = 2.0 * gc1 * (vi - v1) - ic1eq[ch];
        ic2eq[ch] = 2.0 * gc2 * (v2) -ic2eq[ch];

        sample = float (-100.0 * vo[ch]);
    }
}
