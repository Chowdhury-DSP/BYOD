#pragma once

#include "RNNAccelerated.h"
#include <pch.h>

template <int numIns, int hiddenSize, int RecurrentLayerType = RecurrentLayerType::LSTMLayer>
class ResampledRNNAccelerated
{
public:
    ResampledRNNAccelerated();
    ResampledRNNAccelerated (ResampledRNNAccelerated&&) noexcept = default;
    ResampledRNNAccelerated& operator= (ResampledRNNAccelerated&&) noexcept = default;

    void initialise (const void* modelData, int modelDataSize, double modelSampleRate);

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    template <bool useResiduals = false>
    void process (juce::dsp::AudioBlock<float>& block, std::span<const float> condition_data = {}) noexcept
    {
        auto processNNInternal = [this, &condition_data] (const chowdsp::BufferView<float>& bufferView)
        {
            mpark::visit ([&bufferView, &condition_data] (auto& model)
                          {
                              if constexpr (numIns == 1)
                              {
                                  jassert (condition_data.empty());
                                  juce::ignoreUnused (condition_data);
                                  model.process (bufferView.getWriteSpan (0), useResiduals);
                              }
                              else
                              {
                                  jassert ((int) condition_data.size() == bufferView.getNumSamples());
                                  model.process_conditioned (bufferView.getWriteSpan (0), condition_data, useResiduals);
                              }
                          },
                          model_variant);
        };

        if (! needsResampling)
        {
            processNNInternal (block);
        }
        else
        {
            auto bufferView = chowdsp::BufferView<float> { block };
            auto blockAtSampleRate = resampler.processIn (bufferView);
            processNNInternal (blockAtSampleRate);
            resampler.processOut (blockAtSampleRate, bufferView);
            bufferView.clear();
        }
    }

private:
#if JUCE_INTEL
    mpark::variant<rnn_sse::RNNAccelerated<numIns, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>,
                   rnn_avx::RNNAccelerated<numIns, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>>
        model_variant;
#elif JUCE_ARM
    mpark::variant<rnn_arm::RNNAccelerated<numIns, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>> model_variant;
#endif

    using ResamplerType = chowdsp::ResamplingTypes::LanczosResampler<8192, 8>;
    chowdsp::ResampledProcess<ResamplerType> resampler;
    bool needsResampling = true;
    double targetSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResampledRNNAccelerated)
};
