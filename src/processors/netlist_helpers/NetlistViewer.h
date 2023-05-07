#pragma once

#include "CircuitQuantity.h"

namespace netlist
{
struct NetlistViewer : Component
{
    explicit NetlistViewer (CircuitQuantityList& quantities);

    void paint (Graphics& g) override;
    void resized() override;

    OwnedArray<std::pair<Label, Label>> labelPairs;
};
}
