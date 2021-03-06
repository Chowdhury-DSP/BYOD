#pragma once

#include "gui/utils/LabelWithCentredEditor.h"

struct PresetSaveInfo
{
    const String name;
    const String category;
    bool isPublic;
    const String presetID;
};

class PresetsSaveDialog : public Component
{
public:
    PresetsSaveDialog();

    void prepareToShow (const chowdsp::Preset* presetToEdit = nullptr, const File& presetFile = {});

    void paint (Graphics& g) override;
    void resized() override;

    std::function<void (const PresetSaveInfo&)> presetSaveCallback;

private:
    bool isSaveMode = true;
    String presetID {};

    LabelWithCentredEditor nameLabel, categoryLabel;
    ToggleButton publicSwitch { "Public" };
    TextButton okButton { "OK" }, cancelButton { "Cancel" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsSaveDialog)
};
