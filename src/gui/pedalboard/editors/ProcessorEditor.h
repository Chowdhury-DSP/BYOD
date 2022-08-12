#pragma once

#include "KnobsComponent.h"
#include "Port.h"
#include "PowerButton.h"
#include "processors/chain/ProcessorChain.h"

class ProcessorEditor : public Component
{
public:
    ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs);
    ~ProcessorEditor() override;

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;

    void addToBoard (class BoardComponent* boardComp);

    BaseProcessor* getProcPtr() const { return &proc; }
    const ProcessorUIOptions& getUIOptions() const { return procUI; }

    Port* getPort (int portIndex, bool isInput);
    juce::Point<int> getPortLocation (int portIndex, bool isInput) const;
    void setConnectionStatus (bool isConnected, int portIndex, bool isInput);
    Colour getColour() const noexcept { return procUI.backgroundColour; }

private:
    void processorSettingsCallback (PopupMenu& menu, PopupMenu::Options& options);
    Port* getPortPrivate (int portIndex, bool isInput) const;

    void resetProcParameters();
    void createReplaceProcMenu (PopupMenu& menu);

    rocket::signal<void(const BaseProcessor&)> showInfoComp;
    rocket::signal<void(ProcessorEditor&, const MouseEvent&, const juce::Point<int>&)> editorDragged;
    rocket::signal<void(const ProcessorEditor&)> duplicateProcessor;
    rocket::scoped_connection_container connections;

    BaseProcessor& proc;
    ProcessorChain& procChain;

    ProcessorUIOptions& procUI;
    Colour contrastColour;

    KnobsComponent knobs;
    PowerButton powerButton;
    DrawableButton xButton { "", DrawableButton::ButtonStyle::ImageFitted };

    OwnedArray<Port> inputPorts;
    OwnedArray<Port> outputPorts;

    juce::Point<int> mouseDownOffset;

    DrawableButton settingsButton { "Settings", DrawableButton::ImageFitted };

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
};
