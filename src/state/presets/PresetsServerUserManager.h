#pragma once

#include <pch.h>

class PresetsServerUserManager
{
    CREATE_LISTENER (Listener,
                     listeners,
                     virtual void presetLoginStatusChanged() {})
public:
    PresetsServerUserManager();
    ~PresetsServerUserManager();

    bool getIsLoggedIn() const { return isLoggedIn; }

    void attemptToLogIn (const String& username, const String& password);

    void createNewUser (const String& username, const String& password);

    void logOut();

    String getUsername() const { return username; }

private:
    bool isLoggedIn = false;
    String username {};
    String password {};

    using SettingID = chowdsp::GlobalPluginSettings::SettingID;
    static constexpr SettingID userNameSettingID = "user_name";
    static constexpr SettingID userTokenSettingID = "user_token";

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsServerUserManager)
};
