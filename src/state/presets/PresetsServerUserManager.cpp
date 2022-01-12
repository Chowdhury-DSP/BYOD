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

// @TODO: use actual encryption for user token

PresetsServerUserManager::PresetsServerUserManager()
{
    pluginSettings->addProperties ({ { userNameSettingID, String() },
                                     { userTokenSettingID, String() } });

    auto initialUsername = pluginSettings->getProperty<String> (userNameSettingID);

    auto passwordEncoded = pluginSettings->getProperty<String> (userTokenSettingID);
    MemoryOutputStream passwordStream;
    Base64::convertFromBase64 (passwordStream, passwordEncoded);
    auto initialPassword = passwordStream.toString();

    attemptToLogIn (initialUsername, initialPassword);
}

PresetsServerUserManager::~PresetsServerUserManager()
{
    pluginSettings->setProperty (userNameSettingID, username);
    pluginSettings->setProperty (userTokenSettingID, Base64::toBase64 (password));
}

void PresetsServerUserManager::attemptToLogIn (const String& newUsername, const String& newPassword)
{
    if (! isUsernamePasswordPairValid (newUsername, newPassword))
    {
        isLoggedIn = false;
        return;
    }

    using namespace PresetsServerCommunication;
    auto response = sendServerRequest (CommType::validate_user, newUsername, newPassword);

    // @TODO: show alert message on failure

    if (response.contains ("OK"))
    {
        username = newUsername;
        password = newPassword;
        isLoggedIn = true;
    }
}

void PresetsServerUserManager::createNewUser (const String& newUsername, const String& newPassword)
{
    if (! isUsernamePasswordPairValid (newUsername, newPassword))
        return;

    using namespace PresetsServerCommunication;
    auto response = sendServerRequest (CommType::register_user, newUsername, newPassword);

    // @TODO: show alert message on failure

    if (response.contains ("Successfully added user"))
        attemptToLogIn (newUsername, newPassword);
}

void PresetsServerUserManager::logOut()
{
    username = {};
    password = {};
    isLoggedIn = false;
}
