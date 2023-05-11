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

    struct ComponentLabel : Label
    {
        using Label::Label;

        void mouseDoubleClick (const MouseEvent& e) override
        {
            if (onDoubleClick != nullptr)
            {
                onDoubleClick();
                return;
            }

            Label::mouseDoubleClick (e);
        }

        std::function<void()> onDoubleClick = nullptr;
    };

    OwnedArray<std::pair<ComponentLabel, Label>> labelPairs;
    std::unique_ptr<juce::Drawable> schematicSVG;
};
} // namespace netlist
