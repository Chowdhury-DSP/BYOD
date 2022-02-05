#include "PresetsServerSyncManager.h"
#include "PresetInfoHelpers.h"
#include "PresetsServerCommunication.h"

void PresetsServerSyncManager::syncLocalPresetsToServer (const std::vector<const chowdsp::Preset*>& presets, std::vector<AddedPresetInfo>& addedPresetInfo)
{
    Logger::writeToLog ("Syncing local user presets to server");

    using namespace PresetsServerCommunication;
    for (auto* preset : presets)
    {
        if (preset == nullptr)
        {
            jassertfalse;
            continue;
        }

        const auto presetRequestInfo = PresetRequestInfo {
            userManager->getUsername(),
            userManager->getPassword(),
            preset->getName(),
            preset->toXml()->toString(),
            PresetInfoHelpers::getIsPublic (*preset),
            PresetInfoHelpers::getPresetID (*preset),
        };

        if (PresetInfoHelpers::getPresetID (*preset).isEmpty())
        {
            // preset doesn't have an ID yet, so we need to add it:
            auto response = sendAddPresetRequest (presetRequestInfo);

            const String addedStr = "Added successfully: ";
            response = parseMessageResponse (response);
            if (response.contains (addedStr))
                addedPresetInfo.emplace_back (preset, response.fromLastOccurrenceOf (addedStr, false, false));
        }
        else
        {
            // preset has an ID, so we should update it:
            sendUpdatePresetRequest (presetRequestInfo);
        }
    }
}

void PresetsServerSyncManager::syncServerPresetsToLocal (std::vector<chowdsp::Preset>& serverPresets)
{
    Logger::writeToLog ("Syncing user presets from server");
    using namespace PresetsServerCommunication;

    const auto& username = userManager->getUsername();
    auto response = sendServerRequest (CommType::get_presets, username, userManager->getPassword());

    auto presetsReturned = parseMessageResponse (response);
    if (! (presetsReturned.containsChar ('{') && presetsReturned.containsChar ('}')))
    {
        NativeMessageBox::showOkCancelBox (MessageBoxIconType::WarningIcon, "Presets sync failed!", "Unable to fetch presets from server!");
        serverPresets.clear();
        return;
    }

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

            // make sure we don't have duplicates
            // @TODO: figure out why there are duplicates on the server
            if (sst::cpputils::contains_if (serverPresets, [&serverPreset] (const auto& preset)
                                            { return serverPreset.getName() == preset.getName(); }))
                continue;

            serverPresets.push_back (std::move (serverPreset));
        }
    }
    catch (...)
    {
        Logger::writeToLog ("Exception happened while trying to sync server presets!");
        NativeMessageBox::showOkCancelBox (MessageBoxIconType::WarningIcon, "Presets sync failed!", {});

        serverPresets.clear();
        return;
    }
}
