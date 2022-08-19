#pragma once

#include <pch.h>

class BYOD;
class SettingsButton : public DrawableButton,
                       public chowdsp::TrackedByBroadcasters
{
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;

public:
    SettingsButton (const BYOD& processor, chowdsp::OpenGLHelper* openGLHelper);

    void globalSettingChanged (SettingID settingID);

private:
    void showSettingsMenu();
    void cableVizMenu (PopupMenu& menu, int itemID);
    void defaultZoomMenu (PopupMenu& menu, int itemID);
    void openGLManu (PopupMenu& menu, int itemID);
    void copyDiagnosticInfo();

    const BYOD& proc;
    chowdsp::OpenGLHelper* openGLHelper;

    chowdsp::SharedPluginSettings pluginSettings;
    chowdsp::SharedLNFAllocator lnfAllocator;

    static constexpr SettingID openglID = "use_opengl";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsButton)
};
