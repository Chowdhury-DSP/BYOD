#pragma once

#include <pch.h>

class SettingsButton : public DrawableButton
{
public:
    explicit SettingsButton (const AudioProcessor& processor);
    ~SettingsButton() override;

private:
    void showSettingsMenu();
    void defaultZoomMenu (PopupMenu& menu, int itemID);
    void copyDiagnosticInfo();

    const AudioProcessor& proc;

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsButton)
};

class SettingsButtonItem : public foleys::GuiItem
{
public:
    FOLEYS_DECLARE_GUI_FACTORY (SettingsButtonItem)

    SettingsButtonItem (foleys::MagicGUIBuilder& builder, const ValueTree& node) : foleys::GuiItem (builder, node)
    {
        button = std::make_unique<SettingsButton> (*builder.getMagicState().getProcessor());

        addAndMakeVisible (button.get());
    }

    void update() override {}

    Component* getWrappedComponent() override
    {
        return button.get();
    }

private:
    std::unique_ptr<SettingsButton> button;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsButtonItem)
};
