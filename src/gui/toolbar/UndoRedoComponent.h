#pragma once

#include <pch.h>

class UndoRedoComponent : public Component
{
public:
    explicit UndoRedoComponent (UndoManager& undoManager);

    void resized() override;

private:
    void updateButtonEnablementStates (const UndoManager& um);

    DrawableButton undoButton { "Undo", DrawableButton::ImageStretched };
    DrawableButton redoButton { "Redo", DrawableButton::ImageStretched };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UndoRedoComponent)
};
