#pragma once

#include <memory>
#include <span>

namespace RTNeural
{
enum class SampleRateCorrectionMode;
}

template <int inputSize, int hiddenSize, template <typename, int, int, RTNeural::SampleRateCorrectionMode> typename RecurrentLayerType, RTNeural::SampleRateCorrectionMode SRCMode, typename Arch>
class RNNAccelerated
{
public:
    RNNAccelerated();
    ~RNNAccelerated();

    RNNAccelerated (const RNNAccelerated&) = delete;
    RNNAccelerated& operator= (const RNNAccelerated&) = delete;
    RNNAccelerated (RNNAccelerated&&) noexcept = delete;
    RNNAccelerated& operator= (RNNAccelerated&&) noexcept = delete;

    void initialise (const nlohmann::json& weights_json);

    void prepare (int rnnDelaySamples);
    void prepare (float rnnDelaySamples);
    void reset();

    void process (std::span<float> buffer, bool useResiduals = false) noexcept;
    void process_conditioned (std::span<float> buffer, std::span<const float> condition, bool useResiduals = false) noexcept;

private:
    struct Internal;
    Internal* internal = nullptr;

    static constexpr size_t max_model_size = 40000;
    alignas (Arch::alignment()) char internal_data[max_model_size] {};
};
