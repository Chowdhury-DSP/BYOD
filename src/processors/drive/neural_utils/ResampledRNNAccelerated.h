#pragma once

#include "ResampledRNN.h"
#include <mpark/variant.hpp>

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType = RTNeural::LSTMLayerT>
class ResampledRNNAccelerated
{
public:
    ResampledRNNAccelerated();

    void initialise (const void* modelData, int modelDataSize, double modelSampleRate);

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    template <bool useResiduals = false>
    void process (juce::dsp::AudioBlock<float>& block)
    {
        mpark::visit ([&] (auto& model)
                      { model.template process<useResiduals> (block); },
                      model_variant);
    }

private:
    mpark::variant<ResampledRNN<hiddenSize, RecurrentLayerType>
#if JUCE_INTEL
                   ,
                   ResampledRNN<hiddenSize, RecurrentLayerType, xsimd::fma3<xsimd::avx2>>
#endif
                   >
        model_variant;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResampledRNNAccelerated)
};
