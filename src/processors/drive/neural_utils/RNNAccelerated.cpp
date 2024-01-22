#include "RNNAccelerated.h"

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#if ! defined(__has_warning) || __has_warning("-Wunsafe-buffer-usage")
#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#endif
#endif

#include <RTNeural/RTNeural.h>
#include <math_approx/math_approx.hpp>

struct RNNMathsProvider
{
    template <typename T>
    static T tanh (T x)
    {
        return math_approx::tanh<7> (x);
    }

    template <typename T>
    static T sigmoid (T x)
    {
        return math_approx::sigmoid_exp<5, true> (x);
    }
};

#include "model_loaders.h"

#if __clang__
#pragma GCC diagnostic pop
#endif

#if (__MMX__ || __SSE__ || __amd64__) && BYOD_COMPILING_WITH_AVX // INTEL + AVX
namespace rnn_avx
#else
namespace rnn_sse_arm
#endif
{
#if ! (XSIMD_WITH_NEON && BYOD_COMPILING_WITH_AVX)

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
struct RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::Internal
{
    using RecurrentLayerTypeComplete = std::conditional_t<RecurrentLayerType == RecurrentLayerType::LSTMLayer,
                                                          RTNEURAL_NAMESPACE::LSTMLayerT<float, inputSize, hiddenSize, (RTNEURAL_NAMESPACE::SampleRateCorrectionMode) SRCMode, RNNMathsProvider>,
                                                          RTNEURAL_NAMESPACE::GRULayerT<float, inputSize, hiddenSize, (RTNEURAL_NAMESPACE::SampleRateCorrectionMode) SRCMode, RNNMathsProvider>>;
    using DenseLayerType = RTNEURAL_NAMESPACE::DenseT<float, hiddenSize, 1>;
    RTNEURAL_NAMESPACE::ModelT<float, inputSize, 1, RecurrentLayerTypeComplete, DenseLayerType> model;
};

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::RNNAccelerated()
{
    static_assert (sizeof (Internal) <= max_model_size);
    internal = new (internal_data) Internal();
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::~RNNAccelerated()
{
    internal->~Internal();
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::initialise (const nlohmann::json& weights_json)
{
    // @TODO: handle GRU models if needed...
    model_loaders::loadLSTMModel (internal->model, weights_json);
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::prepare ([[maybe_unused]] int rnnDelaySamples)
{
    if constexpr (SRCMode == (int) RTNEURAL_NAMESPACE::SampleRateCorrectionMode::NoInterp)
    {
        internal->model.template get<0>().prepare (rnnDelaySamples);
        internal->model.reset();
    }
}

template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::prepare ([[maybe_unused]] float rnnDelaySamples)
{
    if constexpr (SRCMode == (int) RTNEURAL_NAMESPACE::SampleRateCorrectionMode::LinInterp)
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
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::process (std::span<float> buffer, bool useResiduals) noexcept
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
void RNNAccelerated<inputSize, hiddenSize, RecurrentLayerType, SRCMode>::process_conditioned (std::span<float> buffer, std::span<const float> condition, bool useResiduals) noexcept
{
    alignas (alignment) float input_vec[xsimd::batch<float>::size] {};
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

template class RNNAccelerated<1, 28, RecurrentLayerType::LSTMLayer, (int) RTNEURAL_NAMESPACE::SampleRateCorrectionMode::NoInterp>; // MetalFace
template class RNNAccelerated<2, 24, RecurrentLayerType::LSTMLayer, (int) RTNEURAL_NAMESPACE::SampleRateCorrectionMode::NoInterp>; // BassFace
template class RNNAccelerated<1, 40, RecurrentLayerType::LSTMLayer, (int) RTNEURAL_NAMESPACE::SampleRateCorrectionMode::LinInterp>; // GuitarML (no-cond)
template class RNNAccelerated<2, 40, RecurrentLayerType::LSTMLayer, (int) RTNEURAL_NAMESPACE::SampleRateCorrectionMode::LinInterp>; // GuitarML (cond)
#endif // NEON + AVX
}
