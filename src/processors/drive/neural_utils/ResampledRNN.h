#pragma once

#include <pch.h>

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType = RTNeural::LSTMLayerT>
class ResampledRNN
{
public:
    ResampledRNN() = default;
    ResampledRNN (ResampledRNN&&) noexcept = default;
    ResampledRNN& operator= (ResampledRNN&&) noexcept = default;

    void initialise (const void* modelData, int modelDataSize, double modelSampleRate);

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    template <bool useRedisuals = false>
    void process (juce::dsp::AudioBlock<float>& block)
    {
        auto&& processBlock = oversampler->processSamplesUp (block);

        const auto numSamples = (int) processBlock.getNumSamples();
        auto* x = processBlock.getChannelPointer (0);

        if constexpr (useRedisuals)
        {
            for (int i = 0; i < numSamples; ++i)
                x[i] += model.forward (&x[i]);
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
                x[i] = model.forward (&x[i]);
        }

        oversampler->processSamplesDown (block);
    }

private:
    static constexpr auto DefaultSRCMode = RTNeural::SampleRateCorrectionMode::LinInterp;
    using RecurrentLayerTypeComplete = RecurrentLayerType<float, 1, hiddenSize, DefaultSRCMode>;
    using DenseLayerType = RTNeural::DenseT<float, hiddenSize, 1>;
    RTNeural::ModelT<float, 1, 1, RecurrentLayerTypeComplete, DenseLayerType> model;

    std::unique_ptr<dsp::Oversampling<float>> oversampler;
    double targetSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResampledRNN)
};
