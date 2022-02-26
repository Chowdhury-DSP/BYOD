#pragma once

#include <pch.h>

template <int hiddenSize, typename ResamplerType = chowdsp::ResamplingTypes::LanczosResampler<>>
class MetalFaceRNN
{
public:
    MetalFaceRNN() = default;

    void initialise (const void* modelData, int modelDataSize, double modelSampleRate);

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

private:
    using RecurrentLayerType = RTNeural::LSTMLayerT<float, 1, hiddenSize>;
    using DenseLayerType = RTNeural::DenseT<float, hiddenSize, 1>;
    RTNeural::ModelT<float, 1, 1, RecurrentLayerType, DenseLayerType> model;

    chowdsp::ResampledProcess<ResamplerType> resampler;

    double targetSampleRate = 48000.0;
    bool needsResampling = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MetalFaceRNN)
};
