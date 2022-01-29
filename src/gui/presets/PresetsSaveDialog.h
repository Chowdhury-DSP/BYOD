#pragma once

#include <pch.h>

class PresetsSaveDialog : public Component
{
public:
    PresetsSaveDialog();

    void prepareToShow (bool shouldSave);

    void paint (Graphics& g) override;
    void resized() override;

private:
    bool isSaveMode = true;

    std::unique_ptr<Label> nameLabel, categoryLabel;
    ToggleButton publicSwitch { "Public" };
    TextButton okButton { "OK" }, cancelButton { "Cancel" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsSaveDialog)
};
