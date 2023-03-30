#pragma once

#include <pch.h>

enum class FlapjackClipMode
{
    HardClip = 1,
    AlgSigmoid,
    Asymm,
};

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
    template <FlapjackClipMode clipMode>
    inline void compute() noexcept
    {
        wdft::rtype_detail::RtypeScatter (S_matrix, a_vec, b_vec);

        static constexpr auto v_size = xsimd::batch<float>::size;
        for (int i = 0; i < numPorts; i += v_size)
        {
            const auto a_batch = xsimd::load_aligned (a_vec.data() + i);
            const auto b_batch = xsimd::load_aligned (b_vec.data() + i);
            auto voltage = -0.5f * (a_batch + b_batch);

            const auto sigmoid = [](auto x, float offset)
            {
                static constexpr auto r4_5 = 1.0f / 4.5f;
                const auto rdenominator = xsimd::rsqrt (1.0f + 0.75f * chowdsp::Power::ipow<2> ((x - offset) * r4_5));
                return (x - offset) * rdenominator + 4.5f;
            };

            if constexpr (clipMode == FlapjackClipMode::HardClip)
            {
                voltage = xsimd::clip (voltage, xsimd::broadcast (0.0f), xsimd::broadcast (9.0f));
            }
            else if constexpr (clipMode == FlapjackClipMode::AlgSigmoid)
            {
                voltage = sigmoid (voltage, 4.5f);
            }
            else if constexpr (clipMode == FlapjackClipMode::Asymm)
            {
//                static constexpr auto A = -3.7f;
//                static constexpr auto O = gcem::tanh (A);
//                static constexpr auto K = 200.0f / (O + 1.0f);
//                voltage = K * (chowdsp::Math::algebraicSigmoid (chowdsp::PowApprox::exp (0.1f * voltage - 4.5f) - A) + O);
                voltage = sigmoid (voltage, 4.8f);
            }

            xsimd::store_aligned (b_vec.data() + i, -2.0f * voltage - a_batch);
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
