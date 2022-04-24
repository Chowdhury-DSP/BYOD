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

#if BYOD_BUILD_PRESET_SERVER
    int addPresetServerMenuOptions (int optionID);
#endif

private:
    template <typename ActionType>
    int addPresetMenuItem (PopupMenu* menu, int optionID, const String& itemText, ActionType&& action);

    int addBasicPresetOptions (PopupMenu* menu, int optionID);
    int addPresetShareOptions (PopupMenu* menu, int optionID);
    int addPresetFolderOptions (PopupMenu* menu, int optionID);

    void loadFromFileBrowser();
    void selectedPresetChanged() override;
    void savePreset (const PresetSaveInfo& saveInfo);

#if BYOD_BUILD_PRESET_SERVER
    void updatePresetsToUpdate (PresetManager::PresetUpdateList&);
    void syncServerPresetsToLocal();
#endif

    PresetManager& presetManager;

    chowdsp::WindowInPlugin<PresetsSaveDialog> saveWindow;
    chowdsp::WindowInPlugin<PresetSearchWindow> searchWindow;

    std::shared_ptr<FileChooser> fileChooser;

#if BYOD_BUILD_PRESET_SERVER
    chowdsp::WindowInPlugin<PresetsLoginDialog> loginWindow;
    chowdsp::WindowInPlugin<PresetsSyncDialog> syncWindow;

    SharedPresetsServerUserManager userManager;
    SharedPresetsServerJobPool jobPool;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsComp)
};
