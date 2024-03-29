#pragma once

#include "KnobsComponent.h"
#include "Port.h"
#include "PowerButton.h"
#include "processors/chain/ProcessorChain.h"

class ProcessorEditor : public Component
{
public:
    ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs, chowdsp::HostContextProvider& hostContextProvider);
    ~ProcessorEditor() override;

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;

    void addToBoard (class BoardComponent* boardComp);

    BaseProcessor* getProcPtr() const { return &proc; }
    const ProcessorUIOptions& getUIOptions() const { return procUI; }

    Port* getPort (int portIndex, bool isInput);
    const Port* getPort (int portIndex, bool isInput) const;
    juce::Point<int> getPortLocation (int portIndex, bool isInput) const;
    void setConnectionStatus (bool isConnected, int portIndex, bool isInput);
    Colour getColour() const noexcept { return procUI.backgroundColour; }
    void toggleParamsEnabledOnInputConnectionChange (int inputPortIndex, bool isConnected);

private:
    void processorSettingsCallback (PopupMenu& menu, PopupMenu::Options& options);
    Port* getPortPrivate (int portIndex, bool isInput) const;

    void resetProcParameters();
    void createReplaceProcMenu (PopupMenu& menu);

    chowdsp::Broadcaster<void (const BaseProcessor&)> showInfoCompBroadcaster;
    chowdsp::Broadcaster<void (ProcessorEditor&, const MouseEvent&, const juce::Point<int>&, bool /* dragEnd */)> editorDraggedBroadcaster;
    chowdsp::Broadcaster<void (const ProcessorEditor&)> duplicateProcessorBroadcaster;
    chowdsp::ScopedCallbackList broadcasterCallbacks;

    BaseProcessor& proc;
    ProcessorChain& procChain;

    const ProcessorUIOptions& procUI;
    Colour contrastColour;

    KnobsComponent knobs;
    PowerButton powerButton;
    DrawableButton xButton { "", DrawableButton::ButtonStyle::ImageFitted };

    OwnedArray<Port> inputPorts;
    OwnedArray<Port> outputPorts;

    juce::Point<int> mouseDownOffset;

    DrawableButton settingsButton { "Settings", DrawableButton::ImageFitted };

    chowdsp::ScopedCallback uiOptionsChangedCallback;

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
};
