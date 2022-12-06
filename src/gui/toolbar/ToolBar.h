#pragma once

#include "GlobalParamControls.h"
#include "SettingsButton.h"
#include "UndoRedoComponent.h"
#include "presets/PresetsComp.h"

class BYOD;
class ToolBar : public Component
{
public:
    ToolBar (BYOD& plugin, chowdsp::HostContextProvider& hostContextProvider);

    void paint (Graphics& g) override;
    void resized() override;

private:
    UndoRedoComponent undoRedoComp;
    GlobalParamControls globalParamControls;

    SettingsButton settingsButton;
    chowdsp::CPUMeter cpuMeter;

    PresetsComp presetsComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToolBar)
};
