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
