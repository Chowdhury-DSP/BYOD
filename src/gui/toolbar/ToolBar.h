#pragma once

#include "CPUMeter.h"
#include "GlobalParamControls.h"
#include "SettingsButton.h"
#include "UndoRedoComponent.h"
#include "presets/PresetsComp.h"

class BYOD;
class ToolBar : public Component
{
public:
    explicit ToolBar (BYOD& plugin);

    void paint (Graphics& g) override;
    void resized() override;

private:
    UndoRedoComponent undoRedoComp;
    GlobalParamControls globalParamControls;

    SettingsButton settingsButton;
    CPUMeter cpuMeter;

    PresetsComp presetsComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToolBar)
};
