#pragma once

#include <RTNeural/RTNeural.h>

namespace model_loaders
{
using Vec2d = std::vector<std::vector<float>>;
inline Vec2d transpose (const Vec2d& x)
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
}

template <typename ModelType>
void loadLSTMModel (ModelType& model, const nlohmann::json& weights_json)
{
    const auto& state_dict = weights_json.at ("state_dict");
    RTNEURAL_NAMESPACE::torch_helpers::loadLSTM<float> (state_dict, "rec.", model.template get<0>());
    RTNEURAL_NAMESPACE::torch_helpers::loadDense<float> (state_dict, "lin.", model.template get<1>());
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
    RTNEURAL_NAMESPACE::json_parser::loadGRU<float> (gru, gru_weights);
    RTNEURAL_NAMESPACE::modelt_detail::loadLayer<float> (dense, layer_idx, dense_layer_json, "dense", 1, false);
}
} // namespace model_loaders
