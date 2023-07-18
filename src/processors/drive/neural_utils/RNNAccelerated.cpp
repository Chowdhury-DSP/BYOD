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

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <RTNeural/RTNeural.h>

#include "model_loaders.h"

#if __clang__
#pragma GCC diagnostic pop
#endif

#if (__aarch64__ || __arm__)
namespace rnn_arm
{
#elif __AVX__ || (_MSC_VER && BYOD_COMPILING_WITH_AVX)
namespace rnn_avx
{
#elif __SSE__ || (_MSC_VER && ! BYOD_COMPILING_WITH_AVX)
namespace rnn_sse
{
#else
#error "Unknown or un-supported platform!"
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

template class RNNAccelerated<1, 28, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::NoInterp>; // MetalFace
template class RNNAccelerated<2, 24, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::NoInterp>; // BassFace
template class RNNAccelerated<1, 40, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>; // GuitarML (no-cond)
template class RNNAccelerated<2, 40, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>; // GuitarML (cond)
#endif // NEON + AVX
}
