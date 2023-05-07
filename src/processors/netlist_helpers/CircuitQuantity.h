#pragma once

#include <pch.h>

namespace netlist
{
struct CircuitQuantity
{
    enum Type
    {
        Resistance,
        Capacitance,
    };

    float value;
    float defaultValue;
    float minValue;
    float maxValue;
    Type type;
    std::string name;

    using Setter = juce::dsp::FixedSizeFunction<128, void (const CircuitQuantity&, void*)>;
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

    std::vector<CircuitQuantity> quantities;
};
} // namespace netlist
