#include "PresetsServerSyncManager.h"
#include "PresetsServerCommunication.h"

void PresetsServerSyncManager::syncLocalPresetsToServer (const std::vector<const chowdsp::Preset*> presets)
{
    Logger::writeToLog ("Syncing local user presets to server");

    using namespace PresetsServerCommunication;
    for (const auto* preset : presets)
    {
        auto response = sendAddPresetRequest (userManager->getUsername(),
                                              userManager->getPassword(),
                                              preset->getName(),
                                              preset->toXml()->toString());
    }
}

void PresetsServerSyncManager::syncServerPresetsToLocal (std::vector<chowdsp::Preset>& serverPresets)
{
    Logger::writeToLog ("Syncing user presets from server");
    using namespace PresetsServerCommunication;

    const auto& username = userManager->getUsername();
    auto response = sendServerRequest (CommType::get_presets, username, userManager->getPassword());

    auto presetsReturned = response.fromFirstOccurrenceOf ("{", true, false);
    try
    {
        chowdsp::json presetsJson = chowdsp::json::parse (presetsReturned.toStdString());
        for (auto& [_, presetInfo] : presetsJson.items())
        {
            if (presetInfo["owner"].get<String>() != username)
                continue;

            auto presetXML = XmlDocument::parse (presetInfo["data"].get<String>());
            auto serverPreset = chowdsp::Preset (presetXML.get());
            if (! serverPreset.isValid())
                continue;

            serverPresets.push_back (std::move (serverPreset));
        }
    }
    catch (...)
    {
        Logger::writeToLog ("Exception happened while trying to sync server presets!");
        jassertfalse; // Something bad happened when syncing server presets

        serverPresets.clear();
        return;
    }
}
