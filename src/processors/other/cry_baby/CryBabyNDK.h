#pragma once

#include <modules/Eigen/Eigen/Dense>
#include <modules/Eigen/Eigen/Sparse>

struct CryBabyNDK
{
    // State Vectors
    std::array<Eigen::Vector<double, 6>, 2> x_n;
    std::array<Eigen::Vector<double, 4>, 2> v_n;

    // NDK matrices
    Eigen::Matrix<double, 6, 6> A_mat;
    Eigen::Matrix<double, 6, 2> B_mat;
    Eigen::Matrix<double, 6, 4> C_mat;
    Eigen::Matrix<double, 1, 6> D_mat;
    Eigen::Matrix<double, 1, 2> E_mat;
    Eigen::Matrix<double, 1, 4> F_mat;
    Eigen::Matrix<double, 4, 6> G_mat;
    Eigen::Matrix<double, 4, 2> H_mat;
    Eigen::Matrix<double, 4, 4> K_mat;

    // Intermediate matrices for NDK matrix updates;
    Eigen::Matrix<double, 2, 2> Q;
    Eigen::Matrix<double, 6, 2> Ux;
    Eigen::Matrix<double, 1, 2> Uo;
    Eigen::Matrix<double, 4, 2> Un;
    Eigen::Matrix<double, 2, 2> Uu;
    Eigen::Matrix<double, 6, 6> A0_mat;
    Eigen::Matrix<double, 6, 2> B0_mat;
    Eigen::Matrix<double, 6, 4> C0_mat;
    Eigen::Matrix<double, 1, 6> D0_mat;
    Eigen::Matrix<double, 1, 2> E0_mat;
    Eigen::Matrix<double, 1, 4> F0_mat;
    Eigen::Matrix<double, 4, 6> G0_mat;
    Eigen::Matrix<double, 4, 2> H0_mat;
    Eigen::Matrix<double, 4, 4> K0_mat;
    Eigen::Matrix<double, 6, 6> two_Z_Gx;

    double prev_alpha = 0.0;

    void prepare (double fs, double initial_alpha);
    void process_channel (std::span<float> x, size_t channel_index) noexcept;

    void set_alpha (double new_alpha) noexcept;
};
