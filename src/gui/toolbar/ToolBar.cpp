#include "ToolBar.h"
#include "BYOD.h"
#include "gui/GUIConstants.h"

namespace GUIColours = GUIConstants::Colours;

ToolBar::ToolBar (BYOD& plugin, HostContextProvider& hostContextProvider) : undoRedoComp (plugin.getUndoManager()),
                                                                            globalParamControls (plugin.getVTS(),
                                                                                                 hostContextProvider,
                                                                                                 plugin.getOversampling()),
                                                                            settingsButton (plugin, plugin.getOpenGLHelper()),
                                                                            cpuMeter (plugin.getLoadMeasurer()),
                                                                            presetsComp (reinterpret_cast<PresetManager&> (plugin.getPresetManager()))
{
    addAndMakeVisible (undoRedoComp);
    addAndMakeVisible (globalParamControls);

    addAndMakeVisible (settingsButton);

    addAndMakeVisible (cpuMeter);
    cpuMeter.setColour (ProgressBar::backgroundColourId, Colours::black);
    cpuMeter.setColour (ProgressBar::foregroundColourId, GUIColours::progressBarColour);

    addAndMakeVisible (presetsComp);
    presetsComp.setColour (chowdsp::PresetsComp::textHighlightColourID, GUIColours::titleTextColour);
}

void ToolBar::paint (Graphics& g)
{
    g.fillAll (GUIColours::barBackgroundShade);
}

void ToolBar::resized()
{
    auto bounds = getLocalBounds().reduced (4, 0);

    const auto undoRedoWidth = proportionOfHeight (1.75f);
    undoRedoComp.setBounds (bounds.removeFromLeft (undoRedoWidth));

    bounds.removeFromLeft (3);
    globalParamControls.setBounds (bounds.removeFromLeft (proportionOfWidth (0.6f)));
    bounds.removeFromLeft (3);

    settingsButton.setBounds (bounds.removeFromRight (getHeight()).reduced (4));
    cpuMeter.setBounds (bounds.removeFromRight (proportionOfWidth (0.04f)).reduced (3, 6));

    presetsComp.setBounds (bounds.reduced (0, 5));
}
