/*
 * This file was generated on 2023-06-16 16:46:49.109814
 * using the command: `/Users/jatin/ChowDSP/Research/NDK-Framework/generate_ndk_cpp.py cry_baby_ndk_config.json`
 */

#include <pch.h>

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4459)

#include "CryBabyNDK.h"

namespace CryBabyComponents
{
// START USER ENTRIES
constexpr auto R1 = 68.0e3;
constexpr auto R2 = 22.0e3;
constexpr auto R3 = 390.0;
constexpr auto R4 = 470.0e3;
constexpr auto R5 = 470.0e3;
constexpr auto R6 = 1.5e3;
constexpr auto R7 = 33.0e3;
constexpr auto R8 = 82.0e3;
constexpr auto R9 = 10.0e3;
constexpr auto R10 = 1.0e3;
constexpr auto C1 = 10.0e-9;
constexpr auto C2 = 4.7e-6;
constexpr auto C3 = 10.0e-9;
constexpr auto C4 = 220.0e-9;
constexpr auto C5 = 220.0e-9;
constexpr auto L1 = 500.0e-3;
constexpr auto Vcc = 9.0;
constexpr auto Vt = 26.0e-3;
constexpr auto Is_Q1 = 20.3e-15;
constexpr auto BetaF_Q1 = 1430.0;
constexpr auto AlphaF_Q1 = (1.0 + BetaF_Q1) / BetaF_Q1;
constexpr auto BetaR_Q1 = 4.0;
constexpr auto Is_Q2 = 20.3e-15;
constexpr auto BetaF_Q2 = 1430.0;
constexpr auto AlphaF_Q2 = (1.0 + BetaF_Q2) / BetaF_Q2;
constexpr auto BetaR_Q2 = 4.0;
// END USER ENTRIES
} // namespace CryBabyComponents

void CryBabyNDK::reset (T fs)
{
    using namespace CryBabyComponents;
    const auto Gr = Eigen::DiagonalMatrix<T, num_resistors> { (T) 1 / R1, (T) 1 / R2, (T) 1 / R3, (T) 1 / R4, (T) 1 / R5, (T) 1 / R6, (T) 1 / R7, (T) 1 / R8, (T) 1 / R9, (T) 1 / R10 };
    const auto Gx = Eigen::DiagonalMatrix<T, num_states> { (T) 2 * fs * C1, (T) 2 * fs * C2, (T) 2 * fs * C3, (T) 2 * fs * C4, (T) 2 * fs * C5, (T) 1 / ((T) 2 * fs * L1) };
    const auto Z = Eigen::DiagonalMatrix<T, num_states> { 1, 1, 1, 1, 1, -1 };

    // Set up component-defined matrices
    const Eigen::Matrix<T, num_resistors, num_nodes> Nr {
        { +1, -1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, -1, +0, +0, +0, +0, +0, +0, +0, +0, +1 },
        { +0, +0, +0, +0, +1, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +1, +0, -1, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +1, +0, +0, +0, +0, +0, -1, +0, +0, +0 },
        { +0, +0, +1, +0, +0, +0, -1, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +1, -1, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +1, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +1, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, -1, +0, +1 },
    };

    const Eigen::Matrix<T, num_pots, num_nodes> Nv {
        { +0, +0, +0, +0, +0, +0, +0, +0, +1, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +1, -1, +0, +0, +0, +0 },
    };

    const Eigen::Matrix<T, num_states, num_nodes> Nx {
        { +0, -1, +1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +1, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +1, +0, +0, +0, +0, -1, +0 },
        { +0, +0, +0, +1, +0, +0, +0, -1, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +0, +1, -1, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +1, -1, +0, +0, +0, +0, +0, +0 },
    };

    const Eigen::Matrix<T, num_voltages, num_nodes> Nu {
        { +1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +1 },
    };

    const Eigen::Matrix<T, num_nl_ports, num_nodes> Nn {
        { +0, +0, -1, +1, +0, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +1, -1, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +0, +0, -1, +1, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +1, -1, +0 },
    };

    const Eigen::Matrix<T, num_outputs, num_nodes> No {
        { +0, +0, +0, +0, +0, +0, +0, +1, +0, +0, +0, +0, +0 },
    };

    static constexpr auto S_dim = num_nodes + num_voltages;
    Eigen::Matrix<T, num_states, S_dim> Nx_0 = Eigen::Matrix<T, num_states, S_dim>::Zero();
    Nx_0.block<num_states, num_nodes> (0, 0) = Nx;
    Eigen::Matrix<T, num_nl_ports, S_dim> Nn_0 = Eigen::Matrix<T, num_nl_ports, S_dim>::Zero();
    Nn_0.block<num_nl_ports, num_nodes> (0, 0) = Nn;
    Eigen::Matrix<T, num_outputs, S_dim> No_0 = Eigen::Matrix<T, num_outputs, S_dim>::Zero();
    No_0.block<num_outputs, num_nodes> (0, 0) = No;
    Eigen::Matrix<T, num_pots, S_dim> Nv_0 = Eigen::Matrix<T, num_pots, S_dim>::Zero();
    Nv_0.block<num_pots, num_nodes> (0, 0) = Nv;
    Eigen::Matrix<T, num_voltages, S_dim> Zero_I = Eigen::Matrix<T, num_voltages, S_dim>::Zero();
    Zero_I.block<2, 2> (0, 13).setIdentity();

    Eigen::Matrix<T, S_dim, S_dim> S0_mat = Eigen::Matrix<T, S_dim, S_dim>::Zero();
    S0_mat.block<num_nodes, num_nodes> (0, 0) = Nr.transpose() * Gr * Nr + Nx.transpose() * Gx * Nx;
    S0_mat.block<num_nodes, num_voltages> (0, num_nodes) = Nu.transpose();
    S0_mat.block<num_voltages, num_nodes> (num_nodes, 0) = Nu;
    Eigen::Matrix<T, S_dim, S_dim> S0_inv = S0_mat.inverse();

    // Pre-compute NDK and intermediate matrices
    Q = Nv_0 * (S0_inv * Nv_0.transpose());
    Ux = Nx_0 * (S0_inv * Nv_0.transpose());
    Uo = No_0 * (S0_inv * Nv_0.transpose());
    Un = Nn_0 * (S0_inv * Nv_0.transpose());
    Uu = Zero_I * (S0_inv * Nv_0.transpose());
    A0_mat = (T) 2 * (Z * (Gx * (Nx_0 * (S0_inv * Nx_0.transpose())))) - Z.toDenseMatrix();
    B0_mat = (T) 2 * (Z * (Gx * (Nx_0 * (S0_inv * Zero_I.transpose()))));
    C0_mat = (T) 2 * (Z * (Gx * (Nx_0 * (S0_inv * Nn_0.transpose()))));
    D0_mat = No_0 * (S0_inv * Nx_0.transpose());
    E0_mat = No_0 * (S0_inv * Zero_I.transpose());
    F0_mat = No_0 * (S0_inv * Nn_0.transpose());
    G0_mat = Nn_0 * (S0_inv * Nx_0.transpose());
    H0_mat = Nn_0 * (S0_inv * Zero_I.transpose());
    K0_mat = Nn_0 * (S0_inv * Nn_0.transpose());
    two_Z_Gx = (T) 2 * (Z.toDenseMatrix() * Gx.toDenseMatrix());

    // reset state vectors
    for (size_t ch = 0; ch < MAX_NUM_CHANNELS; ++ch)
    {
        x_n[ch].setZero();
        v_n[ch] = Eigen::Vector<T, num_nl_ports> { 3.9, 4.5, 3.9, 4.5 };
    }
}

void CryBabyNDK::update_pots (const std::array<T, num_pots>& pot_values)
{
    using namespace CryBabyComponents;

    Eigen::Matrix<T, num_pots, num_pots> Rv = Eigen::Matrix<T, num_pots, num_pots>::Zero();
    Rv (0, 0) = pot_values[0];
    Rv (1, 1) = pot_values[1];
    Eigen::Matrix<T, num_pots, num_pots> Rv_Q_inv = (Rv + Q).inverse();

    A_mat = A0_mat - (two_Z_Gx * (Ux * (Rv_Q_inv * Ux.transpose())));
    Eigen::Matrix<T, num_states, num_voltages> B_mat = B0_mat - (two_Z_Gx * (Ux * (Rv_Q_inv * Uu.transpose())));
    B_mat_var = B_mat.leftCols<num_voltages_variable>();
    B_u_fix = B_mat.rightCols<num_voltages_constant>() * Eigen::Vector<T, num_voltages_constant> { Vcc };
    C_mat = C0_mat - (two_Z_Gx * (Ux * (Rv_Q_inv * Un.transpose())));
    D_mat = D0_mat - (Uo * (Rv_Q_inv * Ux.transpose()));
    Eigen::Matrix<T, num_outputs, num_voltages> E_mat = E0_mat - (Uo * (Rv_Q_inv * Uu.transpose()));
    E_mat_var = E_mat.leftCols<num_voltages_variable>();
    E_u_fix = E_mat.rightCols<num_voltages_constant>() * Eigen::Vector<T, num_voltages_constant> { Vcc };
    F_mat = F0_mat - (Uo * (Rv_Q_inv * Un.transpose()));
    G_mat = G0_mat - (Un * (Rv_Q_inv * Ux.transpose()));
    Eigen::Matrix<T, num_nl_ports, num_voltages> H_mat = H0_mat - (Un * (Rv_Q_inv * Uu.transpose()));
    H_mat_var = H_mat.leftCols<num_voltages_variable>();
    H_u_fix = H_mat.rightCols<num_voltages_constant>() * Eigen::Vector<T, num_voltages_constant> { Vcc };
    K_mat = K0_mat - (Un * (Rv_Q_inv * Un.transpose()));
}

void CryBabyNDK::process (std::span<float> channel_data, size_t ch) noexcept
{
    using namespace CryBabyComponents;
    Eigen::Vector<T, num_voltages_variable> u_n_var;
    Eigen::Vector<T, num_nl_ports> p_n;
    Eigen::Matrix<T, num_nl_ports, num_nl_ports> Jac = Eigen::Matrix<T, num_nl_ports, num_nl_ports>::Zero();
    Eigen::Vector<T, num_nl_ports> i_n;
    Eigen::Vector<T, num_nl_ports> F_min;
    Eigen::Matrix<T, num_nl_ports, num_nl_ports> A_solve;
    const Eigen::Matrix<T, num_nl_ports, num_nl_ports> eye = Eigen::Matrix<T, num_nl_ports, num_nl_ports>::Identity();
    Eigen::Vector<T, num_nl_ports> delta_v;
    Eigen::Vector<T, num_outputs> y_n;

    for (auto& sample : channel_data)
    {
        u_n_var (0) = (T) sample;
        p_n.noalias() = G_mat * x_n[ch] + H_mat_var * u_n_var + H_u_fix;

        T exp_v1_v0;
        T exp_mv0;
        T exp_v3_v2;
        T exp_mv2;
        const auto calc_currents = [&]
        {
            exp_v1_v0 = std::exp ((v_n[ch](1) - v_n[ch](0)) / Vt);
            exp_mv0 = std::exp (-v_n[ch](0) / Vt);
            i_n (0) = Is_Q1 * ((exp_v1_v0 - (T) 1) / BetaF_Q1 + (exp_mv0 - (T) 1) / BetaR_Q1);
            i_n (1) = -Is_Q1 * (-(exp_mv0 - (T) 1) + AlphaF_Q1 * (exp_v1_v0 - (T) 1));

            exp_v3_v2 = std::exp ((v_n[ch](3) - v_n[ch](2)) / Vt);
            exp_mv2 = std::exp (-v_n[ch](2) / Vt);
            i_n (2) = Is_Q2 * ((exp_v3_v2 - (T) 1) / BetaF_Q2 + (exp_mv2 - (T) 1) / BetaR_Q2);
            i_n (3) = -Is_Q2 * (-(exp_mv2 - (T) 1) + AlphaF_Q2 * (exp_v3_v2 - (T) 1));
        };

        const auto calc_jacobian = [&]
        {
            Jac (0, 0) = (Is_Q1 / Vt) * (-exp_v1_v0 / BetaF_Q1 - exp_mv0 / BetaR_Q1);
            Jac (0, 1) = (Is_Q1 / Vt) * (exp_v1_v0 / BetaF_Q1);
            Jac (1, 0) = (Is_Q1 / Vt) * (-exp_mv0 + AlphaF_Q1 * exp_v1_v0);
            Jac (1, 1) = (Is_Q1 / Vt) * (-AlphaF_Q1 * exp_v1_v0);

            Jac (2, 2) = (Is_Q2 / Vt) * (-exp_v3_v2 / BetaF_Q2 - exp_mv2 / BetaR_Q2);
            Jac (2, 3) = (Is_Q2 / Vt) * (exp_v3_v2 / BetaF_Q2);
            Jac (3, 2) = (Is_Q2 / Vt) * (-exp_mv2 + AlphaF_Q2 * exp_v3_v2);
            Jac (3, 3) = (Is_Q2 / Vt) * (-AlphaF_Q2 * exp_v3_v2);
        };

        T delta;
        int nIters = 0;
        do
        {
            calc_currents();
            calc_jacobian();

            F_min.noalias() = p_n + K_mat * i_n - v_n[ch];
            A_solve.noalias() = K_mat * Jac - eye;
            delta_v.noalias() = A_solve.householderQr().solve (F_min);
            v_n[ch] -= delta_v;
            delta = delta_v.array().abs().sum();
        } while (delta > 1.0e-2 && ++nIters < 8);
        calc_currents();

        y_n.noalias() = D_mat * x_n[ch] + E_mat_var * u_n_var + E_u_fix + F_mat * i_n;
        sample = (float) y_n (0);
        x_n[ch] = A_mat * x_n[ch] + B_mat_var * u_n_var + B_u_fix + C_mat * i_n;
    }
}

JUCE_END_IGNORE_WARNINGS_MSVC
