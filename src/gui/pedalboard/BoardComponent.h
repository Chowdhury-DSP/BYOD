#pragma once

#include "InfoComponent.h"
#include "cables/CableView.h"
#include "editors/ProcessorEditor.h"
#include "gui/utils/LookAndFeels.h"

class BoardComponent final : public Component
{
public:
    BoardComponent (ProcessorChain& procs, chowdsp::HostContextProvider& hostContextProvider);
    ~BoardComponent() override;

    void resized() override;
    void setScaleFactor (float newScaleFactor);
    float getScaleFactor() const;

    void showInfoComp (const BaseProcessor& proc);
    void editorDragged (ProcessorEditor& editor, const MouseEvent& e, const juce::Point<int>& mouseOffset, bool dragEnded);
    bool isDraggingEditor() const noexcept { return currentlyDraggingEditor; }
    void duplicateProcessor (const ProcessorEditor& editor);

    void processorAdded (BaseProcessor* newProc);
    void processorRemoved (const BaseProcessor* proc);
    void refreshConnections() { resized(); }
    void connectionAdded (const ConnectionInfo&) const;
    void connectionRemoved (const ConnectionInfo&) const;

    const OwnedArray<ProcessorEditor>& getEditors() { return processorEditors; }
    ProcessorEditor* findEditorForProcessor (const BaseProcessor* proc) const;

private:
    void showNewProcMenu (PopupMenu& menu, PopupMenu::Options& options, ConnectionInfo* connectionInfo = nullptr);
    void setEditorPosition (ProcessorEditor* editor, Rectangle<int> bounds = {});

    ProcessorChain& procChain;
    chowdsp::ScopedCallbackList callbacks;

    OwnedArray<ProcessorEditor> processorEditors;
    bool currentlyDraggingEditor = false;
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

    chowdsp::SharedLNFAllocator lnfAllocator;

    chowdsp::HostContextProvider& hostContextProvider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
