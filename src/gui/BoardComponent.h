#pragma once

#include "../processors/ProcessorChain.h"
#include "InfoComponent.h"
#include "ProcessorEditor.h"
#include "utils/LookAndFeels.h"

class BoardComponent : public Component,
                       public DragAndDropContainer,
                       public DragAndDropTarget,
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

    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;

    const OwnedArray<ProcessorEditor>& getEditors() { return processorEditors; }

    static constexpr auto yOffset = 35;

private:
    void showNewProcMenu() const;
    void refreshBoardSize();

    ProcessorChain& procChain;

    TextButton newProcButton;
    OwnedArray<ProcessorEditor> processorEditors;
    InfoComponent infoComp;

    SharedResourcePointer<LNFAllocator> lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
