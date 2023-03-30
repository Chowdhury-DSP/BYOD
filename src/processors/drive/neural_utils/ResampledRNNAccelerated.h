#pragma once

#include "RNNAccelerated.h"
#include <pch.h>

template <int hiddenSize, int RecurrentLayerType = RecurrentLayerType::LSTMLayer>
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
    void process (juce::dsp::AudioBlock<float>& block)
    {
        auto processNNInternal = [this] (const chowdsp::BufferView<float>& bufferView)
        {
            mpark::visit ([&bufferView] (auto& model)
                          { model.process (bufferView.getWriteSpan (0), useResiduals); },
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
        }
    }

private:
#if JUCE_INTEL
    mpark::variant<rnn_sse::RNNAccelerated<1, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>,
                   rnn_avx::RNNAccelerated<1, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>>
        model_variant;
#elif JUCE_ARM
    mpark::variant<rnn_arm::RNNAccelerated<1, hiddenSize, RecurrentLayerType, (int) RTNeural::SampleRateCorrectionMode::NoInterp>> model_variant;
#endif

    using ResamplerType = chowdsp::ResamplingTypes::LanczosResampler<8192, 8>;
    chowdsp::ResampledProcess<ResamplerType> resampler;
    bool needsResampling = true;
    double targetSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResampledRNNAccelerated)
};
