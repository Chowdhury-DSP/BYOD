#if _MSC_VER
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

#include <RTNeural/RTNeural.h>

#if _MSC_VER
#else
#pragma GCC diagnostic pop
#endif

#include "model_loaders.h"
#include "RNNAccelerated.h"

#if ! (XSIMD_WITH_NEON && BYOD_COMPILING_WITH_AVX)

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, typename Arch>
struct RNNAccelerated<hiddenSize, RecurrentLayerType, Arch>::Internal
{
    static constexpr auto DefaultSRCMode = RTNeural::SampleRateCorrectionMode::NoInterp;
    using RecurrentLayerTypeComplete = RecurrentLayerType<float, 1, hiddenSize, DefaultSRCMode>;
    using DenseLayerType = RTNeural::DenseT<float, hiddenSize, 1>;
    RTNeural::ModelT<float, 1, 1, RecurrentLayerTypeComplete, DenseLayerType> model;
};

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, typename Arch>
RNNAccelerated<hiddenSize, RecurrentLayerType, Arch>::RNNAccelerated()
{
    static_assert (sizeof (Internal) <= max_model_size);
    internal = new (internal_data) Internal();

//    allocator.construct (internal);
//    internal = std::make_unique<Internal>();
    std::cout << "Using RNN with SIMD width: " << internal->model.template get<0>().outs[0].size << std::endl;
    std::cout << std::endl;
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, typename Arch>
RNNAccelerated<hiddenSize, RecurrentLayerType, Arch>::~RNNAccelerated()
{
    internal->~Internal();
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, typename Arch>
void RNNAccelerated<hiddenSize, RecurrentLayerType, Arch>::initialise (const nlohmann::json& weights_json)
{
    if constexpr (std::is_same_v<typename Internal::RecurrentLayerTypeComplete, RTNeural::GRULayerT<float, 1, 8, Internal::DefaultSRCMode>>) // Centaur model has keras-style weights
        model_loaders::loadGRUModel (internal->model, weights_json);
    else
        model_loaders::loadLSTMModel (internal->model, hiddenSize, weights_json);
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, typename Arch>
void RNNAccelerated<hiddenSize, RecurrentLayerType, Arch>::prepare (int rnnDelaySamples)
{
    internal->model.template get<0>().prepare (rnnDelaySamples);
    internal->model.reset();
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, typename Arch>
void RNNAccelerated<hiddenSize, RecurrentLayerType, Arch>::reset()
{
    internal->model.reset();
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, typename Arch>
void RNNAccelerated<hiddenSize, RecurrentLayerType, Arch>::process (std::span<float> buffer, bool useResiduals) noexcept
{
    if (useResiduals)
    {
        for (auto& x : buffer)
            x += internal->model.forward (&x);
    }
    else
    {
        for (auto& x : buffer)
            x = internal->model.forward (&x);
    }
}

#if XSIMD_WITH_AVX
template class RNNAccelerated<28, RTNeural::LSTMLayerT, xsimd::fma3<xsimd::avx>>; // MetalFace
#elif XSIMD_WITH_SSE4_1
template class RNNAccelerated<28, RTNeural::LSTMLayerT, xsimd::sse4_1>; // MetalFace
#elif XSIMD_WITH_NEON64
template class RNNAccelerated<28, RTNeural::LSTMLayerT, xsimd::neon64>; // MetalFace
#endif
#endif // NEON + AVX
