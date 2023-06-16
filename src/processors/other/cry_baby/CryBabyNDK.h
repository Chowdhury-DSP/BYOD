/*
 * This file was generated on 2023-06-15 22:16:44.057122
 * using the command: `generate_ndk_cpp.py cry_baby_config.json`
 */
#pragma once

// START USER INCLUDES
#include <modules/Eigen/Eigen/Dense>
// END USER INCLUDES

struct CryBabyNDK
{
    // START USER ENTRIES
    static constexpr size_t MAX_NUM_CHANNELS = 2;
    static constexpr double VR1 = 100.0e3;
    // END USER ENTRIES

    using T = double;
    static constexpr int num_nodes = 13;
    static constexpr int num_resistors = 10;
    static constexpr int num_pots = 2;
    static constexpr int num_states = 6;
    static constexpr int num_nl_ports = 4;
    static constexpr int num_voltages = 2;
    static constexpr int num_outputs = 1;

    // State Variables
    std::array<Eigen::Vector<T, num_states>, MAX_NUM_CHANNELS> x_n;
    std::array<Eigen::Vector<T, num_nl_ports>, MAX_NUM_CHANNELS> v_n;

    // NDK Matrices
    Eigen::Matrix<T, num_states, num_states> A_mat;
    Eigen::Matrix<T, num_states, num_voltages> B_mat;
    Eigen::Matrix<T, num_states, num_nl_ports> C_mat;
    Eigen::Matrix<T, num_outputs, num_states> D_mat;
    Eigen::Matrix<T, num_outputs, num_voltages> E_mat;
    Eigen::Matrix<T, num_outputs, num_nl_ports> F_mat;
    Eigen::Matrix<T, num_nl_ports, num_states> G_mat;
    Eigen::Matrix<T, num_nl_ports, num_voltages> H_mat;
    Eigen::Matrix<T, num_nl_ports, num_nl_ports> K_mat;

    // Intermediate matrices for NDK matrix updates;
    Eigen::Matrix<T, num_pots, num_pots> Q;
    Eigen::Matrix<T, num_states, num_pots> Ux;
    Eigen::Matrix<T, num_outputs, num_pots> Uo;
    Eigen::Matrix<T, num_nl_ports, num_pots> Un;
    Eigen::Matrix<T, num_voltages, num_pots> Uu;
    Eigen::Matrix<T, num_states, num_states> A0_mat;
    Eigen::Matrix<T, num_states, num_voltages> B0_mat;
    Eigen::Matrix<T, num_states, num_nl_ports> C0_mat;
    Eigen::Matrix<T, num_outputs, num_states> D0_mat;
    Eigen::Matrix<T, num_outputs, num_voltages> E0_mat;
    Eigen::Matrix<T, num_outputs, num_nl_ports> F0_mat;
    Eigen::Matrix<T, num_nl_ports, num_states> G0_mat;
    Eigen::Matrix<T, num_nl_ports, num_voltages> H0_mat;
    Eigen::Matrix<T, num_nl_ports, num_nl_ports> K0_mat;
    Eigen::Matrix<T, num_states, num_states> two_Z_Gx;

    void reset (T fs);
    void update_pots (const std::array<T, num_pots>& pot_values);
    void process (std::span<float> channel_data, size_t channel_index) noexcept;
};
