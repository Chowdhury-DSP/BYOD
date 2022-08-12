#pragma once

#include "InfoComponent.h"
#include "cables/CableView.h"
#include "editors/ProcessorEditor.h"
#include "gui/utils/LookAndFeels.h"

class BoardComponent final : public Component,
                             private ProcessorChain::Listener
{
public:
    explicit BoardComponent (ProcessorChain& procs);
    ~BoardComponent() override;

    void resized() override;
    void setScaleFactor (float newScaleFactor);

    void showInfoComp (const BaseProcessor& proc);
    void editorDragged (ProcessorEditor& editor, const MouseEvent& e, const juce::Point<int>& mouseOffset);
    void duplicateProcessor (const ProcessorEditor& editor);

    void processorAdded (BaseProcessor* newProc) override;
    void processorRemoved (const BaseProcessor* proc) override;
    void refreshConnections() override { resized(); }

    const OwnedArray<ProcessorEditor>& getEditors() { return processorEditors; }
    ProcessorEditor* findEditorForProcessor (const BaseProcessor* proc) const;

private:
    void showNewProcMenu (PopupMenu& menu, PopupMenu::Options& options);
    void setEditorPosition (ProcessorEditor* editor, Rectangle<int> bounds = {});

    ProcessorChain& procChain;

    OwnedArray<ProcessorEditor> processorEditors;
    InfoComponent infoComp;

    std::unique_ptr<ProcessorEditor> inputEditor;
    std::unique_ptr<ProcessorEditor> outputEditor;

    friend class CableView;
    friend class CableViewConnectionHelper;
    friend class CableViewPortLocationHelper;
    CableView cableView;

    float scaleFactor = 1.0f;

    TextButton newProcButton;
    chowdsp::PopupMenuHelper popupMenu;
    juce::Point<int> nextEditorPosition {};
    bool addingFromNewProcButton = false;
    bool addingFromRightClick = false;

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
