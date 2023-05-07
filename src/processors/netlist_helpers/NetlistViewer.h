#pragma once

#include "CircuitQuantity.h"

class BaseProcessor;
namespace netlist
{
juce::PopupMenu::Item createNetlistViewerPopupMenuItem (BaseProcessor& processor);

struct NetlistViewer : Component
{
    explicit NetlistViewer (CircuitQuantityList& quantities);

    void paint (Graphics& g) override;
    void resized() override;

    OwnedArray<std::pair<Label, Label>> labelPairs;
};
}
