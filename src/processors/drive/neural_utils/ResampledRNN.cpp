#include "ResampledRNN.h"
#include "model_loaders.h"

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode, typename> typename RecurrentLayerType>
void ResampledRNN<hiddenSize, RecurrentLayerType>::initialise (const void* modelData, int modelDataSize, double modelSampleRate)
{
    targetSampleRate = modelSampleRate;

    MemoryInputStream jsonInputStream (modelData, (size_t) modelDataSize, false);
    auto weightsJson = nlohmann::json::parse (jsonInputStream.readEntireStreamAsString().toStdString());

    if constexpr (std::is_same_v<RecurrentLayerTypeComplete, RTNeural::GRULayerT<float, 1, 8, DefaultSRCMode>>) // Centaur model has keras-style weights
        model_loaders::loadGRUModel (model, weightsJson);
    else
        model_loaders::loadLSTMModel (model, hiddenSize, weightsJson);
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode, typename> typename RecurrentLayerType>
void ResampledRNN<hiddenSize, RecurrentLayerType>::prepare (double sampleRate, int samplesPerBlock)
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

    model.template get<0>().prepare (rnnDelaySamples);
    model.reset();
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode, typename> typename RecurrentLayerType>
void ResampledRNN<hiddenSize, RecurrentLayerType>::reset()
{
    resampler.reset();
    model.reset();
}

//=======================================================
template class ResampledRNN<8, RTNeural::GRULayerT>; // Centaur
