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
    bool showWarningView();

    struct ComponentLabel : Label
    {
        using Label::Label;
        void mouseDoubleClick (const MouseEvent& e) override;
        std::function<void()> onDoubleClick = nullptr;
    };

    OwnedArray<std::pair<ComponentLabel, Label>> labelPairs;
    std::unique_ptr<juce::Drawable> schematicSVG;
    Label noteLabel;

    static constexpr chowdsp::GlobalPluginSettings::SettingID netlistWarningShownID { "has_shown_netlist_warning" };
    chowdsp::SharedPluginSettings pluginSettings;
};
} // namespace netlist
