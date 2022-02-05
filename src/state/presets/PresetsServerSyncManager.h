#pragma once

#include "PresetsServerUserManager.h"

class PresetsServerSyncManager
{
public:
    PresetsServerSyncManager() = default;

    using AddedPresetInfo = std::pair<const chowdsp::Preset*, String>;

    void syncLocalPresetsToServer (const std::vector<const chowdsp::Preset*>& presets, std::vector<AddedPresetInfo>& addedPresetInfo);
    void syncServerPresetsToLocal (std::vector<chowdsp::Preset>& serverPresets);

private:
    SharedResourcePointer<PresetsServerUserManager> userManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsServerSyncManager)
};
