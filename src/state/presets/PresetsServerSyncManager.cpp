#include "PresetsServerSyncManager.h"
#include "PresetInfoHelpers.h"
#include "PresetsServerCommunication.h"

namespace
{
const String presetAddedStr = "Added successfully: ";
const String notConnectedStr = "Unable to connect!";
} // namespace

void PresetsServerSyncManager::syncLocalPresetsToServer (const std::vector<const chowdsp::Preset*>& presets,
                                                         std::vector<AddedPresetInfo>& addedPresetInfo,
                                                         const std::function<void (int, int)>& updateProgressCallback)
{
    Logger::writeToLog ("Syncing local user presets to server");

    using namespace PresetsServerCommunication;

    const auto numPresets = (int) presets.size();
    for (auto [index, preset] : sst::cpputils::enumerate (presets))
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

        String response {};
        if (PresetInfoHelpers::getPresetID (*preset).isEmpty())
        {
            // preset doesn't have an ID yet, so we need to add it:
            response = sendAddPresetRequest (presetRequestInfo);

            response = parseMessageResponse (response);
            if (response.contains (presetAddedStr))
                addedPresetInfo.emplace_back (preset, response.fromLastOccurrenceOf (presetAddedStr, false, false));
        }
        else
        {
            // preset has an ID, so we should update it:
            response = sendUpdatePresetRequest (presetRequestInfo);
            response = parseMessageResponse (response);
        }

        if (response.contains (notConnectedStr))
        {
            MessageManager::callAsync ([]
                                       { NativeMessageBox::showOkCancelBox (MessageBoxIconType::WarningIcon, "Preset Syncing failed!", notConnectedStr); });
            break;
        }

        updateProgressCallback ((int) index, numPresets);
    }
}

void PresetsServerSyncManager::syncServerPresetsToLocal (std::vector<chowdsp::Preset>& serverPresets)
{
    Logger::writeToLog ("Syncing user presets from server");
    using namespace PresetsServerCommunication;

    const auto& username = userManager->getUsername();
    auto response = sendServerRequest (CommType::get_presets, username, userManager->getPassword());

    auto presetsReturned = parseMessageResponse (response);
    if (presetsReturned == "null")
    {
        NativeMessageBox::showOkCancelBox (MessageBoxIconType::WarningIcon, "Presets sync failed!", "No presets for this user exist on the server!");
        serverPresets.clear();
        return;
    }

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
