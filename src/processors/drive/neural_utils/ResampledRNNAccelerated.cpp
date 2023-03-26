#include "ResampledRNNAccelerated.h"

#if ! (JUCE_ARM && BYOD_COMPILING_WITH_AVX)
template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType>
ResampledRNNAccelerated<hiddenSize, RecurrentLayerType>::ResampledRNNAccelerated()
{
#if JUCE_INTEL
    if (xsimd::available_architectures().fma3_avx2) // move down to AVX after XSIMD fixes it
    {
        juce::Logger::writeToLog ("Using RNN model with AVX SIMD instructions!");
        model_variant.template emplace<ResampledRNN<hiddenSize, RecurrentLayerType, xsimd::fma3<xsimd::avx2>>>();
    }
#endif
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType>
void ResampledRNNAccelerated<hiddenSize, RecurrentLayerType>::initialise (const void* modelData, int modelDataSize, double modelSampleRate)
{
    mpark::visit ([&] (auto& model)
                  { model.initialise (modelData, modelDataSize, modelSampleRate); },
                  model_variant);
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType>
void ResampledRNNAccelerated<hiddenSize, RecurrentLayerType>::prepare (double sampleRate, int samplesPerBlock)
{
    mpark::visit ([&] (auto& model)
                  { model.prepare (sampleRate, samplesPerBlock); },
                  model_variant);
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType>
void ResampledRNNAccelerated<hiddenSize, RecurrentLayerType>::reset()
{
    mpark::visit ([&] (auto& model)
                  { model.reset(); },
                  model_variant);
}

template class ResampledRNNAccelerated<28, RTNeural::LSTMLayerT>; // MetalFace

#include "ResampledRNN.cpp"
#if XSIMD_WITH_FMA3_AVX2
static_assert (RTNEURAL_DEFAULT_ALIGNMENT == 32);
static_assert (xsimd::batch<float>::size == 8);
template class ResampledRNN<28, RTNeural::LSTMLayerT, xsimd::fma3<xsimd::avx2>>; // MetalFace
#else
template class ResampledRNN<28, RTNeural::LSTMLayerT>; // MetalFace
#endif
#endif
