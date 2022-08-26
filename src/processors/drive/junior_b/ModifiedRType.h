#pragma once

#include <pch.h>

namespace chowdsp::wdft
{
namespace rtype_detail
{
    /** Implementation for float/double. */
    template <typename T, int nRows, int nCols>
    inline typename std::enable_if<std::is_same<float, T>::value || std::is_same<double, T>::value, void>::type
        VecMatMult (const Matrix<T, nRows, nCols>& S_, const Array<T, nCols>& a_, Array<T, nRows>& b_)
    {
        // input matrix (S) of size nCols x nRows
        // input vector (a) of size 1 x nRows
        // output vector (b) of size 1 x nCols

#if 0 // defined(XSIMD_HPP)
        // @TODO: actually implement this...
#else // No SIMD
        for (int r = 0; r < nRows; ++r)
            b_[r] = (T) 0;

        for (int c = 0; c < nCols; ++c)
        {
            for (int r = 0; r < nRows; ++r)
                b_[r] += S_[c][r] * a_[c];
        }
#endif // SIMD options
    }

}

template <typename T, int numUpPorts, typename ImpedanceCalculator, typename... PortTypes>
class RtypeAdaptorMultN : public BaseWDF
{
public:
    /** Number of ports connected to RtypeAdaptor */
    static constexpr auto numDownPorts = int (sizeof...(PortTypes));

    explicit RtypeAdaptorMultN (std::tuple<PortTypes&...> dps) : downPorts (dps)
    {
        for (int i = 0; i < numDownPorts; i++)
        {
            b_down_vec[i] = (T) 0;
            a_down_vec[i] = (T) 0;
        }

        for (int i = 0; i < numUpPorts; i++)
            b_up_vec[i] = (T) 0;

        rtype_detail::forEachInTuple ([&] (auto& port, size_t) { port.connectToParent (this); },
                                      downPorts);
    }

    /** Re-computes the port impedance at the adapted upward-facing port */
    void calcImpedance() override
    {
        ImpedanceCalculator::calcImpedance (*this);
    }

    constexpr auto getPortImpedances()
    {
        std::array<T, numDownPorts> portImpedances {};
        rtype_detail::forEachInTuple ([&] (auto& port, size_t i) { portImpedances[i] = port.wdf.R; },
                                      downPorts);

        return portImpedances;
    }

    /** Use this function to set the scattering matrix data. */
    void setSMatrixData (const T (&mat)[numUpPorts + numDownPorts][numUpPorts + numDownPorts])
    {
        for (int i = 0; i < numUpPorts; ++i)
            for (int j = 0; j < numDownPorts; ++j)
                S12[j][i] = mat[i][j + numUpPorts];

        for (int i = 0; i < numDownPorts; ++i)
            for (int j = 0; j < numUpPorts + numDownPorts; ++j)
                S21_S22[j][i] = mat[i + numUpPorts][j];
    }

    /** Computes the incident wave. */
    inline void incident (const T* downWave) noexcept
    {
        // set up vector for mat mul
        T mat_mul_vec alignas (CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT)[numUpPorts + numDownPorts];

        for (int i = 0; i < numUpPorts; ++i)
            mat_mul_vec[i] = downWave[i];

        for (int i = 0; i < numDownPorts; ++i)
            mat_mul_vec[numUpPorts + i] = a_down_vec[i];

        rtype_detail::VecMatMult<T, numDownPorts, numUpPorts + numDownPorts> (S21_S22, mat_mul_vec, b_down_vec);
        rtype_detail::forEachInTuple ([&] (auto& port, size_t i) {
                                          port.incident (b_down_vec[i]); },
                                      downPorts);
    }

    /** Computes the reflected wave */
    inline T* reflected() noexcept
    {
        rtype_detail::forEachInTuple ([&] (auto& port, size_t i) {
                                          a_down_vec[i] = port.reflected(); },
                                      downPorts);

        rtype_detail::VecMatMult<T, numUpPorts, numDownPorts> (S12, a_down_vec, b_up_vec);
        return b_up_vec;
    }

private:
    std::tuple<PortTypes&...> downPorts; // tuple of ports connected to RtypeAdaptor

    rtype_detail::Matrix<T, numUpPorts, numDownPorts> S12;
    rtype_detail::Matrix<T, numDownPorts, numUpPorts + numDownPorts> S21_S22;

    T a_down_vec alignas (CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT)[numDownPorts]; // vector of inputs to Rport from down the tree
    T b_down_vec alignas (CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT)[numDownPorts]; // vector of outputs from Rport to down the tree
    T b_up_vec alignas (CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT)[numUpPorts]; // vector of outputs from Rport to down the tree
};
}
