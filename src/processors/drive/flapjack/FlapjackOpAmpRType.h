#pragma once

#include <pch.h>

template <typename T, typename ImpedanceCalculator, typename... PortTypes>
class FlapjackOpAmpRType : public wdft::RootWDF
{
public:
    /** Number of ports connected to RootRtypeAdaptor */
    static constexpr auto numPorts = int (sizeof...(PortTypes));

    explicit FlapjackOpAmpRType (PortTypes&... dps) : downPorts (std::tie (dps...))
    {
        b_vec.clear();
        a_vec.clear();

        wdft::rtype_detail::forEachInTuple ([&] (auto& port, size_t) { port.connectToParent (this); },
                                      downPorts);
    }

    /** Recomputes internal variables based on the incoming impedances */
    void calcImpedance() override
    {
        ImpedanceCalculator::calcImpedance (*this);
    }

    constexpr auto getPortImpedances()
    {
        std::array<T, numPorts> portImpedances {};
        wdft::rtype_detail::forEachInTuple ([&] (auto& port, size_t i) { portImpedances[i] = port.wdf.R; },
                                      downPorts);

        return portImpedances;
    }

    /** Use this function to set the scattering matrix data. */
    void setSMatrixData (const T (&mat)[numPorts][numPorts])
    {
        for (int i = 0; i < numPorts; ++i)
            for (int j = 0; j < numPorts; ++j)
                S_matrix[j][i] = mat[i][j];
    }

    /** Computes both the incident and reflected waves at this root node. */
    inline void compute() noexcept
    {
        wdft::rtype_detail::RtypeScatter (S_matrix, a_vec, b_vec);

        for (int i = 0; i < numPorts; ++i)
        {
            auto voltage = 0.5f * (a_vec[i] + b_vec[i]);
            voltage = std::clamp (voltage, -9.0f, 0.0f);
            b_vec[i] = 2.0f * voltage - a_vec[i];
        }

        wdft::rtype_detail::forEachInTuple ([&] (auto& port, size_t i) {
                                          port.incident (b_vec[i]);
                                          a_vec[i] = port.reflected(); },
                                      downPorts);
    }

private:
    std::tuple<PortTypes&...> downPorts; // tuple of ports connected to RtypeAdaptor

    wdft::rtype_detail::Matrix<T, numPorts> S_matrix; // square matrix representing S
    wdft::rtype_detail::AlignedArray<T, numPorts> a_vec; // temp matrix of inputs to Rport
    wdft::rtype_detail::AlignedArray<T, numPorts> b_vec; // temp matrix of outputs from Rport
};
