#pragma once

#include <pch.h>

struct FuzzFaceSolver
{
    double fs = 48000.0;

    double gr4 = 1.0 / 100.0e3;
    double Rf = 1.0e3;
    double grfp = 1.0 / (0.5 * Rf);
    double grfm = 1.0 / (0.5 * Rf);

    double C1 = 2.2e-6;
    double C2 = 20.0e-6;
    double gc1 = 2.0 * fs * C1;
    double gc2 = 2.0 * fs * C2;

    std::array<double, 2> ic1eq {};
    std::array<double, 2> ic2eq {};
    std::array<double, 2> vo {};
    std::array<double, 2> io {};

    void prepare (double sample_rate);
    void reset();

    void set_gain (double gain_01) noexcept;
    void process (std::span<float> data, size_t channel_index) noexcept;
};
