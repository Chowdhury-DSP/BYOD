#pragma once

#include <pch.h>

class UndoRedoComponent : public Component,
                          private ChangeListener

{
public:
    explicit UndoRedoComponent (UndoManager& undoManager);
    ~UndoRedoComponent() override;

    void resized() override;

private:
    void changeListenerCallback (ChangeBroadcaster* source) override;
    void updateButtonEnablementStates();

    UndoManager& undoManager;

    DrawableButton undoButton { "Undo", DrawableButton::ImageStretched };
    DrawableButton redoButton { "Redo", DrawableButton::ImageStretched };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UndoRedoComponent)
};
