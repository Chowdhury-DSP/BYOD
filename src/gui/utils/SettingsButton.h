#pragma once

#include "BYOD.h"

class SettingsButton : public DrawableButton,
                       private chowdsp::GlobalPluginSettings::Listener
{
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;

public:
    SettingsButton (const BYOD& processor, chowdsp::OpenGLHelper& openGLHelper);
    ~SettingsButton() override;

    void globalSettingChanged (SettingID settingID) final;

private:
    void showSettingsMenu();
    void cableVizMenu (PopupMenu& menu, int itemID);
    void defaultZoomMenu (PopupMenu& menu, int itemID);
    void openGLManu (PopupMenu& menu, int itemID);
    void copyDiagnosticInfo();

    const BYOD& proc;
    chowdsp::OpenGLHelper& openGLHelper;

    chowdsp::SharedPluginSettings pluginSettings;
    chowdsp::SharedLNFAllocator lnfAllocator;

    static constexpr SettingID openglID = "use_opengl";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsButton)
};

class SettingsButtonItem : public foleys::GuiItem
{
public:
    FOLEYS_DECLARE_GUI_FACTORY (SettingsButtonItem)

    SettingsButtonItem (foleys::MagicGUIBuilder& builder, const ValueTree& node) : foleys::GuiItem (builder, node)
    {
        auto* plugin = dynamic_cast<BYOD*> (builder.getMagicState().getProcessor());
        button = std::make_unique<SettingsButton> (*plugin, plugin->getOpenGLHelper());

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
