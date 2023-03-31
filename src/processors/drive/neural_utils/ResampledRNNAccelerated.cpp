#include "ResampledRNNAccelerated.h"

template <int numIns, int hiddenSize, int RecurrentLayerType>
ResampledRNNAccelerated<numIns, hiddenSize, RecurrentLayerType>::ResampledRNNAccelerated()
{
#if JUCE_INTEL
    if (juce::SystemStats::hasAVX() && juce::SystemStats::hasFMA3())
    {
        juce::Logger::writeToLog ("Using RNN model with AVX SIMD instructions!");
        model_variant.template emplace<rnn_avx::RNNAccelerated<numIns, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>>();
    }
#endif
    juce::ignoreUnused (this);
}

template <int numIns, int hiddenSize, int RecurrentLayerType>
void ResampledRNNAccelerated<numIns, hiddenSize, RecurrentLayerType>::initialise (const void* modelData, int modelDataSize, double modelSampleRate)
{
    targetSampleRate = modelSampleRate;

    MemoryInputStream jsonInputStream (modelData, (size_t) modelDataSize, false);
    auto weightsJson = nlohmann::json::parse (jsonInputStream.readEntireStreamAsString().toStdString());

    mpark::visit ([&weightsJson] (auto& model)
                  { model.initialise (weightsJson); },
                  model_variant);
}

template <int numIns, int hiddenSize, int RecurrentLayerType>
void ResampledRNNAccelerated<numIns, hiddenSize, RecurrentLayerType>::prepare (double sampleRate, int samplesPerBlock)
{
    const auto [resampleRatio, rnnDelaySamples] = [] (auto curFs, auto targetFs)
    {
        if (curFs == targetFs)
            return std::make_pair (1.0, 1);

        if (curFs > targetFs)
        {
            const auto delaySamples = std::ceil (curFs / targetFs);
            return std::make_pair (delaySamples * targetFs / curFs, (int) delaySamples);
        }

        // curFs < targetFs
        return std::make_pair (targetFs / curFs, 1);
    }(sampleRate, targetSampleRate);

    needsResampling = resampleRatio != 1.0;
    resampler.prepareWithTargetSampleRate ({ sampleRate, (uint32) samplesPerBlock, 1 }, sampleRate * resampleRatio);

    mpark::visit ([delaySamples = rnnDelaySamples] (auto& model)
                  { model.prepare (delaySamples); },
                  model_variant);
}

template <int numIns, int hiddenSize, int RecurrentLayerType>
void ResampledRNNAccelerated<numIns, hiddenSize, RecurrentLayerType>::reset()
{
    resampler.reset();
    mpark::visit ([] (auto& model)
                  { model.reset(); },
                  model_variant);
}

//=======================================================
template class ResampledRNNAccelerated<1, 28>; // MetalFace
template class ResampledRNNAccelerated<2, 24>; // BassFace
