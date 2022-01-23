#pragma once

#include "PresetsLoginDialog.h"
#include "PresetsSyncDialog.h"
#include "state/presets/PresetManager.h"

class PresetsComp : public chowdsp::PresetsComp
{
public:
    explicit PresetsComp (PresetManager& presetMgr);

    void presetListUpdated() final;
    int addPresetServerMenuOptions (int optionID);

    void updatePresetsToUpdate();

private:
    void syncServerPresetsToLocal();

    PresetManager& presetManager;

    chowdsp::WindowInPlugin<PresetsLoginDialog> loginWindow;
    chowdsp::WindowInPlugin<PresetsSyncDialog> syncWindow;

    PresetManager::PresetUpdateList presetsToUpdate;

    SharedResourcePointer<PresetsServerUserManager> userManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsComp)
};
