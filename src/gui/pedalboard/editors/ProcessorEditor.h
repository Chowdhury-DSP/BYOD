#pragma once

#include "PowerButton.h"
#include "KnobsComponent.h"
#include "Port.h"
#include "processors/chain/ProcessorChain.h"

class ProcessorEditor : public Component
{
    // clang-format off
    CREATE_LISTENER (
        Listener,
        listeners,
        virtual void showInfoComp (const BaseProcessor&) {}\
        virtual void editorDragged (ProcessorEditor&, const MouseEvent&, const Point<int>&) {}\
        virtual void duplicateProcessor (const ProcessorEditor&) {}\
    )
    // clang-format on
public:
    ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs);
    ~ProcessorEditor() override;

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;

    BaseProcessor* getProcPtr() const { return &proc; }
    const ProcessorUIOptions& getUIOptions() const { return procUI; }

    Port* getPort (int portIndex, bool isInput);
    Point<int> getPortLocation (int portIndex, bool isInput) const;
    void setConnectionStatus (bool isConnected, int portIndex, bool isInput);
    Colour getColour() const noexcept { return procUI.backgroundColour; }

private:
    void processorSettingsCallback (PopupMenu& menu, PopupMenu::Options& options);
    Port* getPortPrivate (int portIndex, bool isInput) const;

    void resetProcParameters();
    void createReplaceProcMenu (PopupMenu& menu);

    BaseProcessor& proc;
    ProcessorChain& procChain;

    ProcessorUIOptions& procUI;
    Colour contrastColour;

    KnobsComponent knobs;
    PowerButton powerButton;
    DrawableButton xButton { "", DrawableButton::ButtonStyle::ImageFitted };

    OwnedArray<Port> inputPorts;
    OwnedArray<Port> outputPorts;

    Point<int> mouseDownOffset;

    DrawableButton settingsButton { "Settings", DrawableButton::ImageFitted };

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
};
