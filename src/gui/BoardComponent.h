#pragma once

#include "../processors/ProcessorChain.h"
#include "Cable.h"
#include "InfoComponent.h"
#include "ProcessorEditor.h"
#include "utils/LookAndFeels.h"

class BoardComponent : public Component,
                       private ProcessorChain::Listener,
                       private ProcessorEditor::PortListener
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
    ProcessorEditor* findEditorForProcessor (const BaseProcessor* proc) const;

    void createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e) override;
    void refreshCable (const MouseEvent& e) override;
    void releaseCable (const MouseEvent& e) override;
    void destroyCable (ProcessorEditor* origin, int portIndex) override;

    static constexpr auto yOffset = 35;

private:
    void showNewProcMenu() const;
    void refreshBoardSize();

    ProcessorChain& procChain;

    TextButton newProcButton;
    OwnedArray<ProcessorEditor> processorEditors;
    InfoComponent infoComp;

    std::unique_ptr<ProcessorEditor> inputEditor;
    std::unique_ptr<ProcessorEditor> outputEditor;

    OwnedArray<Cable> cables;
    std::unique_ptr<MouseEvent> cableMouse;

    SharedResourcePointer<LNFAllocator> lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
