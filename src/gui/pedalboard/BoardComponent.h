#pragma once

#include "InfoComponent.h"
#include "cables/CableView.h"
#include "editors/EditorSelector.h"
#include "editors/ProcessorEditor.h"
#include "gui/utils/LookAndFeels.h"

class BoardComponent final : public Component
{
public:
    BoardComponent (ProcessorChain& procs, chowdsp::HostContextProvider& hostContextProvider);
    ~BoardComponent() override;

    void resized() override;
    void paintOverChildren (Graphics& g) override;
    void setScaleFactor (float newScaleFactor);
    float getScaleFactor() const;

    void showInfoComp (const BaseProcessor& proc);
    void editorDeleteRequested (ProcessorEditor& editor);
    void editorDragged (ProcessorEditor& editor, const MouseEvent& e, const juce::Point<int>& mouseOffset, bool dragEnded);
    bool isDraggingEditor() const noexcept { return currentlyDraggingEditor; }
    void duplicateProcessor (const ProcessorEditor& editor);

    void processorAdded (BaseProcessor* newProc);
    void processorRemoved (const BaseProcessor* proc);
    void refreshConnections() { resized(); }
    void connectionAdded (const ConnectionInfo&) const;
    void connectionRemoved (const ConnectionInfo&) const;

    const OwnedArray<ProcessorEditor>& getEditors() const { return processorEditors; }
    auto* getInputProcessorEditor() const { return inputEditor.get(); }
    auto* getOutputProcessorEditor() const { return outputEditor.get(); }
    ProcessorEditor* findEditorForProcessor (const BaseProcessor* proc) const;

private:
    void showNewProcMenu (PopupMenu& menu, PopupMenu::Options& options, juce::Point<int> mousePos, ConnectionInfo* connectionInfo = nullptr);
    void setEditorPosition (ProcessorEditor* editor, juce::Rectangle<int> bounds = {});
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;

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

    EditorSelector editorSelector;
    LassoComponent<ProcessorEditor*> selectorLasso;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
