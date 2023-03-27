#pragma once

#include <memory>
#include <span>

namespace RTNeural
{
enum class SampleRateCorrectionMode;
}

template <int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, typename Arch>
class RNNAccelerated
{
public:
    RNNAccelerated();
    ~RNNAccelerated();

    void initialise (const nlohmann::json& weights_json);

    void prepare (int rnnDelaySamples);
    void reset();

    void process (std::span<float> buffer, bool useResiduals = false) noexcept;

private:
    struct Internal;
    Internal* internal = nullptr;

    static constexpr size_t max_model_size = 20000;
    alignas (32) char internal_data[max_model_size] {};
};
