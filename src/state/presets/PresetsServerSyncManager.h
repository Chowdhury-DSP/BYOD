#if BYOD_BUILD_PRESET_SERVER

#pragma once

#include "PresetsServerUserManager.h"

class PresetsServerSyncManager
{
public:
    PresetsServerSyncManager() = default;

    using AddedPresetInfo = std::pair<const chowdsp::Preset*, String>;

    void syncLocalPresetsToServer (const std::vector<const chowdsp::Preset*>& presets,
                                   std::vector<AddedPresetInfo>& addedPresetInfo,
                                   const std::function<void (int, int)>& updateProgressCallback);
    bool syncServerPresetsToLocal (std::vector<chowdsp::Preset>& serverPresets);

private:
    SharedPresetsServerUserManager userManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsServerSyncManager)
};

#endif // BYOD_BUILD_PRESET_SERVER
