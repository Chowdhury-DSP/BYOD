#pragma once

#include "../processors/ProcessorChain.h"
#include "ProcessorEditor.h"

class BoardComponent : public Component,
                       private ProcessorChain::Listener
{
public:
    BoardComponent (ProcessorChain& procs);
    ~BoardComponent();

    void paint (Graphics& g) override;
    void resized() override;

    int getIdealWidth (int parentWidth = -1) const;
    void processorAdded (BaseProcessor* newProc) override;

private:
    void showNewProcMenu() const;

    ProcessorChain& procChain;

    TextButton newProcButton;
    OwnedArray<ProcessorEditor> processorEditors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
