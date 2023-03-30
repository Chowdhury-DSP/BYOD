#include "RNNAccelerated.h"

#if __AVX__
#define RTNeural RTNeural_avx
#define xsimd xsimd_avx
#elif __SSE__
#define RTNeural RTNeural_sse
#define xsimd xsimd_sse
#else
#define RTNeural RTNeural_arm
#define xsimd xsimd_arm
#endif

#if _MSC_VER
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

#include <RTNeural/RTNeural.h>
#include "model_loaders.h"

#if _MSC_VER
#else
#pragma GCC diagnostic pop
#endif

#if __AVX__
namespace rnn_avx
{
#elif __SSE__
namespace rnn_sse
{
#else
namespace rnn_arm
{
#endif

#if ! (XSIMD_WITH_NEON && BYOD_COMPILING_WITH_AVX)

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
struct RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::Internal
{
    using RecurrentLayerTypeComplete = std::conditional_t<RecurrentLayerType == RecurrentLayerType::LSTMLayer,
          RTNeural::LSTMLayerT<float, inputSize, hiddenSize, (RTNeural::SampleRateCorrectionMode) SRCMode>,
          RTNeural::GRULayerT<float, inputSize, hiddenSize, (RTNeural::SampleRateCorrectionMode) SRCMode>>;
    using DenseLayerType = RTNeural::DenseT<float, hiddenSize, 1>;
    RTNeural::ModelT<float, inputSize, 1, RecurrentLayerTypeComplete, DenseLayerType> model;
};

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::RNNAccelerated()
{
    static_assert (sizeof (Internal) <= max_model_size);
    internal = new (internal_data) Internal();
    //    std::cout << "Using RNN with SIMD width: " << internal->model.template get<0>().outs[0].size << std::endl;
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::~RNNAccelerated()
{
    internal->~Internal();
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::initialise (const nlohmann::json& weights_json)
{
//    if constexpr (::std::is_same_v<typename Internal::RecurrentLayerTypeComplete, RTNeural::GRULayerT<float, 1, 8, SRCMode>>) // Centaur model has keras-style weights
//        model_loaders::loadGRUModel (internal->model, weights_json);
//    else
        model_loaders::loadLSTMModel (internal->model, hiddenSize, weights_json);
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::prepare ([[maybe_unused]] int rnnDelaySamples)
{
    if constexpr (SRCMode == (int) RTNeural::SampleRateCorrectionMode::NoInterp)
    {
        internal->model.template get<0>().prepare (rnnDelaySamples);
        internal->model.reset();
    }
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::prepare ([[maybe_unused]] float rnnDelaySamples)
{
    if constexpr (SRCMode == (int) RTNeural::SampleRateCorrectionMode::LinInterp)
    {
        internal->model.template get<0>().prepare (rnnDelaySamples);
        internal->model.reset();
    }
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::reset()
{
    internal->model.reset();
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::process (::std::span<float> buffer, bool useResiduals) noexcept
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

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::process_conditioned (::std::span<float> buffer, ::std::span<const float> condition, bool useResiduals) noexcept
{
    alignas (RTNEURAL_DEFAULT_ALIGNMENT) float input_vec[::xsimd::batch<float>::size] {};
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

template class RNNAccelerated<1, 28, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::NoInterp>; // MetalFace
template class RNNAccelerated<1, 40, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>; // GuitarML (no-cond)
template class RNNAccelerated<2, 40, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>; // GuitarML (cond)
#endif // NEON + AVX
}
