#include "CryBabyNDK.h"

namespace
{
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
constexpr auto VR1 = 100.0e3;
constexpr auto C1 = 10.0e-9;
constexpr auto C2 = 4.7e-6;
constexpr auto C3 = 10.0e-9;
constexpr auto C4 = 220.0e-9;
constexpr auto C5 = 220.0e-9;
constexpr auto L1 = 500.0e-3;
constexpr auto Vcc = 9.0;
constexpr auto Vt = 26.0e-3;
constexpr auto Is = 20.3e-15;
constexpr auto BetaF = 1430.0;
constexpr auto BetaR = 4.0;
} // namespace

void CryBabyNDK::prepare (double fs, double initial_alpha)
{
    const auto Gr = Eigen::DiagonalMatrix<double, 10> { 1.0 / R1, 1.0 / R2, 1.0 / R3, 1.0 / R4, 1.0 / R5, 1.0 / R6, 1.0 / R7, 1.0 / R8, 1.0 / R9, 1.0 / R10 };
    const auto Gx = Eigen::DiagonalMatrix<double, 6> { 2.0 * fs * C1, 2.0 * fs * C2, 2.0 * fs * C3, 2.0 * fs * C4, 2.0 * fs * C5, 1.0 / (2.0 * fs * L1) };
    const auto Z = Eigen::DiagonalMatrix<double, 6> { 1.0, 1.0, 1.0, 1.0, 1.0, -1.0 };

    const Eigen::Matrix<double, 10, 13> Nr {
        { +0, +0, +0, +0, +0, +0, +0, +0, -1, +0, +1, +0, +0 },
        { -1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +1, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +1, +0, +0, +0, +0, +0 },
        { +1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, -1 },
        { -1, +0, +0, +0, +0, +0, +0, +0, +0, +1, +0, +0, +0 },
        { +0, +0, +0, +1, -1, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, -1, +0, +0, +0, +0, +0, +0, +0, +1 },
        { +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +1 },
        { +0, +0, +0, +0, +0, -1, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, -1, +0, +0, +0, +0, +1, +0 },
    };

    const Eigen::Matrix<double, 2, 13> Nv {
        { +0, +0, +1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +1, -1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0 },
    };

    const Eigen::Matrix<double, 6, 13> Nx {
        { +0, +0, +0, -1, +0, +0, +0, +0, +1, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +1 },
        { +0, +0, +0, +0, +1, -1, +0, +0, +0, +0, +0, +0, +0 },
        { -1, +1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +0, +0, +1, +0, +0, +0, +0, +0, +0, -1, +0, +0, +0 },
        { +0, +0, +0, +0, -1, +0, +0, +0, +0, +0, +0, +0, +1 },
    };

    const Eigen::Matrix<double, 2, 13> Nu {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
    };

    const Eigen::Matrix<double, 4, 13> Nn {
        { +1, +0, +0, -1, +0, +0, +0, +0, +0, +0, +0, +0, +0 },
        { +1, +0, +0, +0, +0, +0, +0, -1, +0, +0, +0, +0, +0 },
        { +0, +0, +0, +0, +0, +0, +1, +0, +0, -1, +0, +0, +0 },
        { +0, +0, +0, +0, +0, -1, +1, +0, +0, +0, +0, +0, +0 },
    };

    const Eigen::Matrix<double, 1, 13> No {
        { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    Eigen::Matrix<double, 6, 15> Nx_0;
    Nx_0.setZero();
    Nx_0.block<6, 13> (0, 0) = Nx;

    Eigen::Matrix<double, 4, 15> Nn_0;
    Nn_0.setZero();
    Nn_0.block<4, 13> (0, 0) = Nn;

    Eigen::Matrix<double, 1, 15> No_0;
    No_0.setZero();
    No_0.block<1, 13> (0, 0) = No;

    Eigen::Matrix<double, 2, 15> Zero_I;
    Zero_I.setZero();
    Zero_I.block<2, 2> (0, 13).setIdentity();

    //============================================
    Eigen::Matrix<double, 15, 15> S0_mat;
    S0_mat.setZero();
    S0_mat.block<13, 13> (0, 0) = Nr.transpose() * Gr * Nr
                                  + Nx.transpose() * Gx * Nx;
    S0_mat.block<13, 2> (0, 13) = Nu.transpose();
    S0_mat.block<2, 13> (13, 0) = Nu;
    Eigen::Matrix<double, 15, 15> S0_inv = S0_mat.inverse();

    Eigen::Matrix<double, 2, 15> Nv_0;
    Nv_0.setZero();
    Nv_0.block<2, 13> (0, 0) = Nv;

    Q = Nv_0 * (S0_inv * Nv_0.transpose());
    Ux = Nx_0 * (S0_inv * Nv_0.transpose());
    Uo = No_0 * (S0_inv * Nv_0.transpose());
    Un = Nn_0 * (S0_inv * Nv_0.transpose());
    Uu = Zero_I * (S0_inv * Nv_0.transpose());

    A0_mat = 2.0 * (Z * (Gx * (Nx_0 * (S0_inv * Nx_0.transpose())))) - Z.toDenseMatrix();
    B0_mat = 2.0 * (Z * (Gx * (Nx_0 * (S0_inv * Zero_I.transpose()))));
    C0_mat = 2.0 * (Z * (Gx * (Nx_0 * (S0_inv * Nn_0.transpose()))));
    D0_mat = No_0 * (S0_inv * Nx_0.transpose());
    E0_mat = No_0 * (S0_inv * Zero_I.transpose());
    F0_mat = No_0 * (S0_inv * Nn_0.transpose());
    G0_mat = Nn_0 * (S0_inv * Nx_0.transpose());
    H0_mat = Nn_0 * (S0_inv * Zero_I.transpose());
    K0_mat = Nn_0 * (S0_inv * Nn_0.transpose());
    two_Z_Gx = 2.0 * (Z.toDenseMatrix() * Gx.toDenseMatrix());

    prev_alpha = -1.0; // force the update!
    set_alpha (initial_alpha);

    //============================================
    // reset state
    for (size_t ch = 0; ch < 2; ++ch)
    {
        x_n[ch].setZero();
//        v_n[ch].setZero();
//        x_n[ch] = Eigen::Vector<double, 6> { -0.00129042068087091,
//                                         0.6066797237167123,
//                                         -0.0064893267388429644,
//                                         -0.19469253168118281,
//                                         -0.19741507823194129,
//                                         -1.5319085469643699E-7 };
        v_n[ch] = Eigen::Vector<double, 4> { 3.9271560942528319, 4.524363916506168, 3.9262980403171812, 4.5429223634080538 };
    }
}

void CryBabyNDK::set_alpha (double alpha) noexcept
{
    if (alpha == prev_alpha)
        return;

    jassert (alpha >= 0.1 && alpha <= 0.99);
    alpha = juce::jlimit (0.1, 0.99, alpha);

    prev_alpha = alpha;

    const auto Rv = Eigen::Matrix<double, 2, 2> { { (1.0 - alpha) * VR1, 0.0 }, { 0.0, alpha * VR1 } };

    Eigen::Matrix<double, 2, 2> Rv_Q_inv = (Rv + Q).inverse();

    A_mat = A0_mat - (two_Z_Gx * (Ux * (Rv_Q_inv * Ux.transpose())));
    B_mat = B0_mat - (two_Z_Gx * (Ux * (Rv_Q_inv * Uu.transpose())));
    C_mat = C0_mat - (two_Z_Gx * (Ux * (Rv_Q_inv * Un.transpose())));
    D_mat = D0_mat - (Uo * (Rv_Q_inv * Ux.transpose()));
    E_mat = E0_mat - (Uo * (Rv_Q_inv * Uu.transpose()));
    F_mat = F0_mat - (Uo * (Rv_Q_inv * Un.transpose()));
    G_mat = G0_mat - (Un * (Rv_Q_inv * Ux.transpose()));
    H_mat = H0_mat - (Un * (Rv_Q_inv * Uu.transpose()));
    K_mat = K0_mat - (Un * (Rv_Q_inv * Un.transpose()));
}

void CryBabyNDK::process_channel (std::span<float> x, size_t ch) noexcept
{
    Eigen::Vector<double, 2> u_n { 0.0, Vcc };

    Eigen::Vector<double, 4> p_n;
    Eigen::Matrix<double, 4, 4> Jac;
    Jac.setZero();
    Eigen::Vector<double, 4> i_n;
    Eigen::Vector<double, 4> F_min;
    Eigen::Matrix<double, 4, 4> A_solve;
    const Eigen::Matrix<double, 4, 4> eye4 = Eigen::Matrix<double, 4, 4>::Identity();
    Eigen::Vector<double, 4> delta_v;
    Eigen::Vector<double, 1> y_n;

    for (auto& sample : x)
    {
        u_n (0) = (double) sample;

        p_n.noalias() = G_mat * x_n[ch] + H_mat * u_n;

        static constexpr auto alphaF = (1 + BetaF) / BetaF;
        double exp_v1_v0;
        double exp_mv0;
        double exp_v3_v2;
        double exp_mv2;
        const auto calc_currents = [&]
        {
            exp_v1_v0 = std::exp ((v_n[ch] (1) - v_n[ch] (0)) / Vt);
            exp_mv0 = std::exp (-v_n[ch] (0) / Vt);
            exp_v3_v2 = std::exp ((v_n[ch] (3) - v_n[ch] (2)) / Vt);
            exp_mv2 = std::exp (-v_n[ch] (2) / Vt);

            i_n (0) = Is * ((exp_v1_v0 - 1.0) / BetaF + (exp_mv0 - 1.0) / BetaR);
            i_n (1) = -Is * (-(exp_mv0 - 1.0) + alphaF * (exp_v1_v0 - 1.0));
            i_n (2) = Is * ((exp_v3_v2 - 1.0) / BetaF + (exp_mv2 - 1.0) / BetaR);
            i_n (3) = -Is * (-(exp_mv2 - 1.0) + alphaF * (exp_v3_v2 - 1.0));
        };

        const auto calc_jacobian = [&]
        {
            Jac (0, 0) = Is / Vt * (-exp_v1_v0 / BetaF - exp_mv0 / BetaR);
            Jac (0, 1) = Is / Vt * (exp_v1_v0 / BetaF);
            Jac (1, 0) = Is / Vt * (-exp_mv0 + alphaF * exp_v1_v0);
            Jac (1, 1) = Is / Vt * (-alphaF * exp_v1_v0);
            Jac (2, 2) = Is / Vt * (-exp_v3_v2 / BetaF - exp_mv2 / BetaR);
            Jac (2, 3) = Is / Vt * (exp_v3_v2 / BetaF);
            Jac (3, 2) = Is / Vt * (-exp_mv2 + alphaF * exp_v3_v2);
            Jac (3, 3) = Is / Vt * (-alphaF * exp_v3_v2);
        };

        double delta;
        int nIters = 0;
        do
        {
            calc_currents();
            calc_jacobian();

            F_min.noalias() = p_n + K_mat * i_n - v_n[ch];
            A_solve.noalias() = K_mat * Jac - eye4;
            delta_v.noalias() = A_solve.householderQr().solve (F_min);
            v_n[ch] -= delta_v;

            delta = delta_v.array().abs().sum();
        } while (delta > 1.0e-2 && ++nIters < 8);

        calc_currents();

        y_n.noalias() = D_mat * x_n[ch] + E_mat * u_n + F_mat * i_n;
        sample = (float) y_n (0);

        x_n[ch] = A_mat * x_n[ch] + B_mat * u_n + C_mat * i_n;
    }
}
