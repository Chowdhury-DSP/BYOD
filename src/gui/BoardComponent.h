#pragma once

#include "../processors/ProcessorChain.h"
#include "InfoComponent.h"
#include "InputEditor.h"
#include "ProcessorEditor.h"
#include "utils/LookAndFeels.h"

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
    void processorRemoved (const BaseProcessor* proc) override;
    void processorMoved (int procToMove, int procInSlot) override;
    void showInfoComp (const BaseProcessor& proc);

    const OwnedArray<ProcessorEditor>& getEditors() { return processorEditors; }

    static constexpr auto yOffset = 35;

private:
    void showNewProcMenu() const;
    void refreshBoardSize();

    ProcessorChain& procChain;

    TextButton newProcButton;
    OwnedArray<ProcessorEditor> processorEditors;
    InfoComponent infoComp;

    InputEditor inputEditor;
    OutputEditor outputEditor;

    SharedResourcePointer<LNFAllocator> lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
