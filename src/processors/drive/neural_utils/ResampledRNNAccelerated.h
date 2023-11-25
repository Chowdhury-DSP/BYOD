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
    void process (std::span<float> block, std::span<const float> condition_data = {}) noexcept
    {
        auto processNNInternal = [this, &condition_data] (std::span<float> data)
        {
            model_variant.visit (
                [&data, &condition_data] (auto& model)
                {
                    if constexpr (numIns == 1)
                    {
                        jassert (condition_data.empty());
                        juce::ignoreUnused (condition_data);
                        model.process (data, useResiduals);
                    }
                    else
                    {
                        jassert (condition_data.size() == data.size());
                        model.process_conditioned (data, condition_data, useResiduals);
                    }
                });
        };

        if (! needsResampling)
        {
            processNNInternal (block);
        }
        else
        {
            auto bufferView = chowdsp::BufferView<float> { block.data(), (int) block.size() };
            auto blockAtSampleRate = resampler.processIn (bufferView);
            processNNInternal (blockAtSampleRate.getWriteSpan (0));
            resampler.processOut (blockAtSampleRate, bufferView);
        }
    }

    template <bool useResiduals = false>
    void process (const juce::dsp::AudioBlock<float>& block, std::span<const float> condition_data = {}) noexcept
    {
        jassert (block.getNumChannels() == 1);
        const auto bufferView = chowdsp::BufferView<float> { block };
        process<useResiduals> (bufferView.getWriteSpan (0), condition_data);
    }

private:
    EA::Variant<rnn_sse_arm::RNNAccelerated<numIns, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>
#if JUCE_INTEL
                ,
                rnn_avx::RNNAccelerated<numIns, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>
#endif
                >
        model_variant;

    using ResamplerType = chowdsp::ResamplingTypes::LanczosResampler<8192, 8>;
    chowdsp::ResampledProcess<ResamplerType> resampler;
    bool needsResampling = true;
    double targetSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResampledRNNAccelerated)
};
