#pragma once

#include "PresetsServerUserManager.h"

class PresetsServerSyncManager
{
public:
    PresetsServerSyncManager() = default;

    void syncLocalPresetsToServer (const std::vector<const chowdsp::Preset*> presets);

private:
    SharedResourcePointer<PresetsServerUserManager> userManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsServerSyncManager)
};
