#pragma once

#include <pch.h>

class PresetsSaveDialog : public Component
{
public:
    PresetsSaveDialog();

    void prepareToShow (const chowdsp::Preset* presetToEdit = nullptr, const File& presetFile = {});

    void paint (Graphics& g) override;
    void resized() override;

    std::function<void (const String& /*name*/, const String& /*category*/, bool /*isPublic*/)> presetSaveCallback;
    std::function<void (const String& /*name*/, const String& /*category*/, bool /*isPublic*/)> presetEditCallback;

private:
    bool isSaveMode = true;

    std::unique_ptr<Label> nameLabel, categoryLabel;
    ToggleButton publicSwitch { "Public" };
    TextButton okButton { "OK" }, cancelButton { "Cancel" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsSaveDialog)
};
