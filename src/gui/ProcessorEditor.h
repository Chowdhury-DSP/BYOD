#pragma once

#include "../processors/ProcessorChain.h"
#include "KnobsComponent.h"

class ProcessorEditor : public Component
{
public:
    ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs);
    ~ProcessorEditor();

    void paint (Graphics& g) override;
    void resized() override;

    const BaseProcessor* getProcPtr() const { return &proc; }

private:
    BaseProcessor& proc;
    ProcessorChain& procChain;
    
    const ProcessorUIOptions& procUI;
    const Colour contrastColour;

    KnobsComponent knobs;
    TextButton xButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
};
