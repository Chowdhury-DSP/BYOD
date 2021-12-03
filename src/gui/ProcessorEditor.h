#pragma once

#include "../processors/chain/ProcessorChain.h"
#include "KnobsComponent.h"
#include "Port.h"
#include "utils/PowerButton.h"

class ProcessorEditor : public Component,
                        private Port::PortListener
{
    // clang-format off
    CREATE_LISTENER (
        PortListener,
        portListeners,
        virtual void createCable (ProcessorEditor* /*origin*/, int /*portIndex*/, const MouseEvent&) {}\
        virtual void refreshCable (const MouseEvent&) {}\
        virtual void releaseCable (const MouseEvent&) {}\
        virtual void destroyCable (ProcessorEditor* /*origin*/, int /*portIndex*/) {}\
    )
    // clang-format on
public:
    ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs, Component* parent);
    ~ProcessorEditor() override;

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;

    BaseProcessor* getProcPtr() const { return &proc; }
    const ProcessorUIOptions& getUIOptions() const { return procUI; }

    void createCable (Port* origin, const MouseEvent& e) override;
    void refreshCable (const MouseEvent& e) override;
    void releaseCable (const MouseEvent& e) override;
    void destroyCable (Port* origin) override;

    Point<int> getPortLocation (int portIndex, bool isInput) const;

private:
    void createReplaceProcMenu();

    BaseProcessor& proc;
    ProcessorChain& procChain;

    const ProcessorUIOptions& procUI;
    Colour contrastColour;

    KnobsComponent knobs;
    PowerButton powerButton;
    DrawableButton xButton { "", DrawableButton::ButtonStyle::ImageFitted };
    DrawableButton swapButton { "", DrawableButton::ButtonStyle::ImageFitted };
    DrawableButton infoButton { "", DrawableButton::ButtonStyle::ImageFitted };

    OwnedArray<Port> inputPorts;
    OwnedArray<Port> outputPorts;

    Point<int> mouseDownOffset;

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
};
