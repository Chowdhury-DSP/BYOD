#pragma once

#include <pch.h>

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode, typename> typename RecurrentLayerType = RTNeural::LSTMLayerT>
class ResampledRNN
{
public:
    ResampledRNN() = default;
    ResampledRNN (ResampledRNN&&) noexcept = default;
    ResampledRNN& operator= (ResampledRNN&&) noexcept = default;

    void initialise (const void* modelData, int modelDataSize, double modelSampleRate);

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    template <bool useResiduals = false>
    void process (juce::dsp::AudioBlock<float>& block)
    {
        auto processNNInternal = [this] (const chowdsp::BufferView<float>& bufferView)
        {
            const auto numSamples = bufferView.getNumSamples();
            auto* x = bufferView.getWritePointer (0);

            if constexpr (useResiduals)
            {
                for (int i = 0; i < numSamples; ++i)
                    x[i] += model.forward (&x[i]);
            }
            else
            {
                for (int i = 0; i < numSamples; ++i)
                    x[i] = model.forward (&x[i]);
            }
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
    static constexpr auto DefaultSRCMode = RTNeural::SampleRateCorrectionMode::NoInterp;
    using RecurrentLayerTypeComplete = RecurrentLayerType<float, 1, hiddenSize, DefaultSRCMode, RTNeural::DefaultMathsProvider>;
    using DenseLayerType = RTNeural::DenseT<float, hiddenSize, 1>;
    RTNeural::ModelT<float, 1, 1, RecurrentLayerTypeComplete, DenseLayerType> model;

    using ResamplerType = chowdsp::ResamplingTypes::LanczosResampler<8192, 8>;
    chowdsp::ResampledProcess<ResamplerType> resampler;
    bool needsResampling = true;
    double targetSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResampledRNN)
};
