#pragma once

#include "state/presets/PresetManager.h"

class PresetsSyncDialog : public Component
{
public:
    PresetsSyncDialog();

    void paint (Graphics& g) override;
    void resized() override;

    void updatePresetsList (const PresetManager::PresetUpdateList& presetsToUpdate);

    std::function<void()> runUpdateCallback = [] {};

private:
    String presetUpdateText;
    TextButton okButton { "OK" }, cancelButton { "Cancel" };

    ListBox presetsList;
    std::unique_ptr<ListBoxModel> listBoxModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsSyncDialog)
};
