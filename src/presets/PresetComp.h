#pragma once

#include "PresetManager.h"

class PresetComp : public Component,
                   private PresetManager::Listener
{
public:
    enum ColourIDs
    {
        backgroundColourId,
        textColourId,
    };

    PresetComp (PresetManager& manager);
    ~PresetComp();

    void paint (Graphics& g) override;
    void resized() override;
    void presetUpdated() override;

    void menuItemAction() const;

private:
    void loadPresetChoices();
    void addPresetOptions();
    void saveUserPreset();

    PresetManager& manager;
    ComboBox presetBox;
    TextEditor presetNameEditor;

    DrawableButton presetsLeft, presetsRight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetComp)
};
