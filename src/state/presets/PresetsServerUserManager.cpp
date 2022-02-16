#include "PresetsServerUserManager.h"
#include "PresetsServerCommunication.h"

namespace
{
bool isUsernamePasswordPairValid (const String& username, const String& password)
{
    bool invalidTokens = username.isEmpty() || password.isEmpty();
    invalidTokens |= username == "User";

    return ! invalidTokens;
}
} // namespace

PresetsServerUserManager::PresetsServerUserManager()
{
    pluginSettings->addProperties ({ { userNameSettingID, String() },
                                     { userTokenSettingID, String() } });

    jobPool->addJob (
        [&]
        {
            auto initialUsername = pluginSettings->getProperty<String> (userNameSettingID);

            auto passwordEncoded = pluginSettings->getProperty<String> (userTokenSettingID);
            MemoryOutputStream passwordStream;
            Base64::convertFromBase64 (passwordStream, passwordEncoded);
            auto initialPassword = passwordStream.toString();

            attemptToLogIn (initialUsername, initialPassword, true);
        });
}

PresetsServerUserManager::~PresetsServerUserManager()
{
    pluginSettings->setProperty (userNameSettingID, username);
    pluginSettings->setProperty (userTokenSettingID, Base64::toBase64 (password));
}

void PresetsServerUserManager::attemptToLogIn (const String& newUsername, const String& newPassword, bool failSilently)
{
    if (! isUsernamePasswordPairValid (newUsername, newPassword))
    {
        if (! failSilently)
            PresetsServerCommunication::showFailureMessage ("Login attempt failed!", "Invalid username/password combo");

        isLoggedIn = false;
        return;
    }

    using namespace PresetsServerCommunication;
    auto response = sendServerRequest (CommType::validate_user, newUsername, newPassword);

    if (response.contains ("OK"))
    {
        username = newUsername;
        password = newPassword;
        isLoggedIn = true;

        listeners.call (&Listener::presetLoginStatusChanged);
        return;
    }

    if (! failSilently)
        PresetsServerCommunication::showFailureMessage ("Login attempt failed!", parseMessageResponse (response));
}

void PresetsServerUserManager::createNewUser (const String& newUsername, const String& newPassword)
{
    if (! isUsernamePasswordPairValid (newUsername, newPassword))
    {
        PresetsServerCommunication::showFailureMessage ("Registration attempt failed!", "Invalid username/password combo");
        return;
    }

    using namespace PresetsServerCommunication;
    auto response = sendServerRequest (CommType::register_user, newUsername, newPassword);

    if (response.contains ("Successfully added user"))
    {
        attemptToLogIn (newUsername, newPassword);
        return;
    }

    PresetsServerCommunication::showFailureMessage ("Registration attempt failed!", parseMessageResponse (response));
}

void PresetsServerUserManager::logOut()
{
    username = {};
    password = {};
    isLoggedIn = false;
}
