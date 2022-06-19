#include "ResampledRNN.h"

namespace
{
using Vec2d = std::vector<std::vector<float>>;
auto transpose = [] (const Vec2d& x) -> Vec2d
{
    auto outer_size = x.size();
    auto inner_size = x[0].size();
    Vec2d y (inner_size, std::vector<float> (outer_size, 0.0f));

    for (size_t i = 0; i < outer_size; ++i)
    {
        for (size_t j = 0; j < inner_size; ++j)
            y[j][i] = x[i][j];
    }

    return std::move (y);
};

template <typename ModelType>
void loadLSTMModel (ModelType& model, int hiddenSize, const nlohmann::json& weights_json)
{
    auto& lstm = model.template get<0>();
    auto& dense = model.template get<1>();

    Vec2d lstm_weights_ih = weights_json["/state_dict/rec.weight_ih_l0"_json_pointer];
    lstm.setWVals (transpose (lstm_weights_ih));

    Vec2d lstm_weights_hh = weights_json["/state_dict/rec.weight_hh_l0"_json_pointer];
    lstm.setUVals (transpose (lstm_weights_hh));

    std::vector<float> lstm_bias_ih = weights_json["/state_dict/rec.bias_ih_l0"_json_pointer];
    std::vector<float> lstm_bias_hh = weights_json["/state_dict/rec.bias_hh_l0"_json_pointer];
    for (int i = 0; i < 4 * hiddenSize; ++i)
        lstm_bias_hh[(size_t) i] += lstm_bias_ih[(size_t) i];
    lstm.setBVals (lstm_bias_hh);

    Vec2d dense_weights = weights_json["/state_dict/lin.weight"_json_pointer];
    dense.setWeights (dense_weights);

    std::vector<float> dense_bias = weights_json["/state_dict/lin.bias"_json_pointer];
    dense.setBias (dense_bias.data());
}

template <typename ModelType>
void loadGRUModel (ModelType& model, const nlohmann::json& weights_json)
{
    auto json_layers = weights_json["layers"];
    const auto gru_layer_json = json_layers.at (0);
    const auto dense_layer_json = json_layers.at (1);

    auto& gru = model.template get<0>();
    auto& dense = model.template get<1>();

    int layer_idx = 0;
    const auto& gru_weights = gru_layer_json["weights"];
    RTNeural::json_parser::loadGRU<float> (gru, gru_weights);
    RTNeural::modelt_detail::loadLayer<float> (dense, layer_idx, dense_layer_json, "dense", 1, false);
}
} // namespace

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType>
void ResampledRNN<hiddenSize, RecurrentLayerType>::initialise (const void* modelData, int modelDataSize, double modelSampleRate)
{
    targetSampleRate = modelSampleRate;

    MemoryInputStream jsonInputStream (modelData, (size_t) modelDataSize, false);
    auto weightsJson = nlohmann::json::parse (jsonInputStream.readEntireStreamAsString().toStdString());

    if constexpr (std::is_same_v<RecurrentLayerTypeComplete, RTNeural::GRULayerT<float, 1, 8, DefaultSRCMode>>) // Centaur model has keras-style weights
        loadGRUModel (model, weightsJson);
    else
        loadLSTMModel (model, hiddenSize, weightsJson);
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType>
void ResampledRNN<hiddenSize, RecurrentLayerType>::prepare (double sampleRate, int samplesPerBlock)
{
    auto resampledSampleRate = sampleRate;
    int resampleOrder = 0;
    for (; resampleOrder <= 4; ++resampleOrder)
    {
        if (resampledSampleRate >= targetSampleRate)
            break;

        resampledSampleRate *= 2.0;
    }

    const auto rnnDelaySamples = resampledSampleRate / targetSampleRate;
    jassert (rnnDelaySamples >= 1.0);

    oversampler = std::make_unique<dsp::Oversampling<float>> (2, (size_t) resampleOrder, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampler->initProcessing ((size_t) samplesPerBlock);

    model.template get<0>().prepare ((float) rnnDelaySamples);
    model.reset();
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType>
void ResampledRNN<hiddenSize, RecurrentLayerType>::reset()
{
    oversampler->reset();
    model.reset();
}

//=======================================================
template class ResampledRNN<20, RTNeural::LSTMLayerT>; // GuitarML
template class ResampledRNN<28, RTNeural::LSTMLayerT>; // MetalFace
template class ResampledRNN<8, RTNeural::GRULayerT>; // Centaur
