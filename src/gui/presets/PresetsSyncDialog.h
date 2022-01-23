#pragma once

#include <pch.h>

class PresetsSyncDialog : public Component
{
public:
    PresetsSyncDialog();

    void paint (Graphics& g) override;
    void resized() override;

    void updatePresetsList (const std::vector<chowdsp::Preset>& presetsToUpdate);

    std::function<void()> runUpdateCallback = [] {};

private:
    String presetUpdateText;
    TextButton okButton { "OK" }, cancelButton { "Cancel" };

    ListBox presetsList;
    std::unique_ptr<ListBoxModel> listBoxModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsSyncDialog)
};
