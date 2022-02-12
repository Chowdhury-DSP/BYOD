#pragma once

#include "PresetSearchWindow.h"
#include "PresetsLoginDialog.h"
#include "PresetsSaveDialog.h"
#include "PresetsSyncDialog.h"
#include "state/presets/PresetManager.h"

class PresetsComp : public chowdsp::PresetsComp
{
public:
    explicit PresetsComp (PresetManager& presetMgr);

    void presetListUpdated() final;
    int createPresetsMenu (int optionID) override;
    int addPresetOptions (int optionID) override;
    int addPresetServerMenuOptions (int optionID);

private:
    void updatePresetsToUpdate (PresetManager::PresetUpdateList&);
    void selectedPresetChanged() override;
    void syncServerPresetsToLocal();
    void savePreset (const PresetSaveInfo& saveInfo);

    PresetManager& presetManager;

    chowdsp::WindowInPlugin<PresetsLoginDialog> loginWindow;
    chowdsp::WindowInPlugin<PresetsSaveDialog> saveWindow;
    chowdsp::WindowInPlugin<PresetSearchWindow> searchWindow;
    chowdsp::WindowInPlugin<PresetsSyncDialog> syncWindow;

    SharedPresetsServerUserManager userManager;
    SharedPresetsServerJobPool jobPool;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsComp)
};
