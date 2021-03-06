#if BYOD_BUILD_PRESET_SERVER

#pragma once

#include "PresetsServerJobPool.h"

class PresetsServerUserManager
{
    CREATE_LISTENER (Listener,
                     listeners,
                     virtual void presetLoginStatusChanged() {})
public:
    PresetsServerUserManager();
    ~PresetsServerUserManager();

    bool getIsLoggedIn() const { return isLoggedIn; }

    void attemptToLogIn (const String& username, const String& password, bool failSilently = false);

    void createNewUser (const String& username, const String& password);

    void logOut();

    String getUsername() const { return username; }
    String getPassword() const { return password; }

private:
    bool isLoggedIn = false;
    String username {};
    String password {};

    using SettingID = chowdsp::GlobalPluginSettings::SettingID;
    static constexpr SettingID userNameSettingID = "user_name";
    static constexpr SettingID userTokenSettingID = "user_token";

    chowdsp::SharedPluginSettings pluginSettings;
    SharedPresetsServerJobPool jobPool;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsServerUserManager)
};

using SharedPresetsServerUserManager = SharedResourcePointer<PresetsServerUserManager>;

#endif // BYOD_BUILD_PRESET_SERVER
