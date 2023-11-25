#pragma once

#include <modules/json/json.hpp>
#include <span>

namespace RecurrentLayerType
{
constexpr int LSTMLayer = 1;
constexpr int GRULayer = 2;
} // namespace RecurrentLayerType

namespace rnn_sse_arm
{
template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
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

    static constexpr size_t max_model_size = 30000;
    static constexpr size_t alignment = 16;
    alignas (alignment) char internal_data[max_model_size] {};
};
} // namespace rnn_sse_arm

#if __MMX__ || __SSE__ || __amd64__ // INTEL
namespace rnn_avx
{
template <int inputSize, int hiddenSize, int RecurrentLayerType, int SRCMode>
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
    static constexpr size_t alignment = 32;
    alignas (alignment) char internal_data[max_model_size] {};
};
} // namespace rnn_avx
#endif
