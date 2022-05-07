#if BYOD_BUILD_PRESET_SERVER

#pragma once

#include "state/presets/PresetManager.h"

class PresetsSyncDialog : public Component
{
public:
    PresetsSyncDialog();
    ~PresetsSyncDialog() override;

    void paint (Graphics& g) override;
    void resized() override;

    void updatePresetsList (PresetManager::PresetUpdateList& presetsToUpdate);

    std::function<void (PresetManager::PresetUpdateList&)> runUpdateCallback = [] (PresetManager::PresetUpdateList&) {};

private:
    String presetUpdateText;
    TextButton okButton { "OK" }, cancelButton { "Cancel" };

    ListBox presetsList;
    std::unique_ptr<ListBoxModel> listBoxModel;

    PresetManager::PresetUpdateList* presetUpdateList = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsSyncDialog)
};

#endif // BYOD_BUILD_PRESET_SERVER
