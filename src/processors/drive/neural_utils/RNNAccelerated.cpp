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

#include "RNNAccelerated.h"
#include "model_loaders.h"

#if ! (XSIMD_WITH_NEON && BYOD_COMPILING_WITH_AVX)

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
struct RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::Internal
{
    using RecurrentLayerTypeComplete = RecurrentLayerType<float, inputSize, hiddenSize, SRCMode>;
    using DenseLayerType = RTNeural::DenseT<float, hiddenSize, 1>;
    RTNeural::ModelT<float, inputSize, 1, RecurrentLayerTypeComplete, DenseLayerType> model;
};

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::RNNAccelerated()
{
    static_assert (sizeof (Internal) <= max_model_size);
    internal = new (internal_data) Internal();
    //    std::cout << "Using RNN with SIMD width: " << internal->model.template get<0>().outs[0].size << std::endl;
}

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::~RNNAccelerated()
{
    internal->~Internal();
}

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::initialise (const nlohmann::json& weights_json)
{
    if constexpr (std::is_same_v<typename Internal::RecurrentLayerTypeComplete, RTNeural::GRULayerT<float, 1, 8, SRCMode>>) // Centaur model has keras-style weights
        model_loaders::loadGRUModel (internal->model, weights_json);
    else
        model_loaders::loadLSTMModel (internal->model, hiddenSize, weights_json);
}

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::prepare ([[maybe_unused]] int rnnDelaySamples)
{
    if constexpr (SRCMode == RTNeural::SampleRateCorrectionMode::NoInterp)
    {
        internal->model.template get<0>().prepare (rnnDelaySamples);
        internal->model.reset();
    }
}

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::prepare ([[maybe_unused]] float rnnDelaySamples)
{
    if constexpr (SRCMode == RTNeural::SampleRateCorrectionMode::LinInterp)
    {
        internal->model.template get<0>().prepare (rnnDelaySamples);
        internal->model.reset();
    }
}

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::reset()
{
    internal->model.reset();
}

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::process (std::span<float> buffer, bool useResiduals) noexcept
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

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode, Arch>::process_conditioned (std::span<float> buffer, std::span<const float> condition, bool useResiduals) noexcept
{
    alignas (RTNEURAL_DEFAULT_ALIGNMENT) float input_vec[xsimd::batch<float>::size] {};
    if (useResiduals)
    {
        for (size_t n = 0; n < buffer.size(); ++n)
        {
            input_vec[0] = buffer[n];
            input_vec[1] = condition[n];
            buffer[n] += internal->model.forward (input_vec);
        }
    }
    else
    {
        for (size_t n = 0; n < buffer.size(); ++n)
        {
            input_vec[0] = buffer[n];
            input_vec[1] = condition[n];
            buffer[n] = internal->model.forward (input_vec);
        }
    }
}

#if XSIMD_WITH_AVX
template class RNNAccelerated<1, 28, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::NoInterp, xsimd::fma3<xsimd::avx>>; // MetalFace
template class RNNAccelerated<1, 40, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::LinInterp, xsimd::fma3<xsimd::avx>>; // GuitarML (no-cond)
template class RNNAccelerated<2, 40, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::LinInterp, xsimd::fma3<xsimd::avx>>; // GuitarML (cond)
#elif XSIMD_WITH_SSE4_1
template class RNNAccelerated<1, 28, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::NoInterp, xsimd::sse4_1>; // MetalFace
template class RNNAccelerated<1, 40, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::LinInterp, xsimd::sse4_1>; // GuitarML (no-cond)
template class RNNAccelerated<2, 40, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::LinInterp, xsimd::sse4_1>; // GuitarML (cond)
#elif XSIMD_WITH_NEON64
template class RNNAccelerated<1, 28, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::NoInterp, xsimd::neon64>; // MetalFace
template class RNNAccelerated<1, 40, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::LinInterp, xsimd::neon64>; // GuitarML (no-cond)
template class RNNAccelerated<2, 40, RTNeural::LSTMLayerT, RTNeural::SampleRateCorrectionMode::LinInterp, xsimd::neon64>; // GuitarML (cond)
#endif
#endif // NEON + AVX
