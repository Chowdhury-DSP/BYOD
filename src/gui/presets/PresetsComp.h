#pragma once

#include "PresetsLoginDialog.h"
#include "WindowInPlugin.h"
#include "state/presets/PresetManager.h"

class PresetsComp : public chowdsp::PresetsComp
{
public:
    explicit PresetsComp (PresetManager& presetMgr);

    void presetListUpdated() final;
    int addPresetServerMenuOptions (int optionID);

private:
    PresetManager& presetManager;

    WindowInPlugin<PresetsLoginDialog> loginWindow;

    SharedResourcePointer<PresetsServerUserManager> userManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsComp)
};
