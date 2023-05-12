#pragma once

#include <pch.h>

class BaseProcessor;
namespace netlist
{
struct CircuitQuantity
{
    enum Type
    {
        Resistance,
        Capacitance,
    };

    using Setter = juce::dsp::FixedSizeFunction<32, void (const CircuitQuantity&)>;

    CircuitQuantity (CircuitQuantity&& other) noexcept;
    CircuitQuantity (float defaultVal, float minVal, float maxVal, Type qType, const std::string& name, Setter&& setterFunc);

    std::atomic<float> value;
    std::atomic_bool needsUpdate { false };

    const float defaultValue;
    const float minValue;
    const float maxValue;
    const Type type;
    const std::string name;

    Setter setter;
};

juce::String toString (const CircuitQuantity& q);
float fromString (const juce::String& str, const CircuitQuantity& q);

struct CircuitQuantityList
{
    auto begin() { return quantities.begin(); }
    auto begin() const { return quantities.begin(); }
    auto end() { return quantities.end(); }
    auto end() const { return quantities.end(); }
    auto size() const noexcept { return quantities.size(); }

    void addResistor (float defaultValue, const std::string& name, CircuitQuantity::Setter&& setter, float minVal = 100.0f, float maxVal = 10.0e6f);
    void addCapacitor (float defaultValue, const std::string& name, CircuitQuantity::Setter&& setter, float minVal = 0.1e-12f, float maxVal = 10.0f);

    [[nodiscard]] const CircuitQuantity* findQuantity (const std::string&) const;

    std::vector<CircuitQuantity> quantities;
    struct SchematicSVGData
    {
        const char* data = nullptr;
        int size = 0;
    } schematicSVG;
};
} // namespace netlist
