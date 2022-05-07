#include "UndoRedoComponent.h"

namespace
{
constexpr int buttonPad = 10;
}

UndoRedoComponent::UndoRedoComponent (UndoManager& undoManager)
{
    undoButton.setImages (Drawable::createFromImageData (BinaryData::undosolid_png, BinaryData::undosolid_pngSize).get());
    addAndMakeVisible (undoButton);
    undoButton.onClick = [this, &undoManager]
    {
        undoManager.undo();
        updateButtonEnablementStates (undoManager);
    };

    redoButton.setImages (Drawable::createFromImageData (BinaryData::redosolid_png, BinaryData::redosolid_pngSize).get());
    addAndMakeVisible (redoButton);
    redoButton.onClick = [this, &undoManager]
    {
        undoManager.redo();
        updateButtonEnablementStates (undoManager);
    };

    updateButtonEnablementStates (undoManager);
}

void UndoRedoComponent::updateButtonEnablementStates (const UndoManager& um)
{
    undoButton.setEnabled (um.canUndo());
    redoButton.setEnabled (um.canRedo());
}

void UndoRedoComponent::resized()
{
    auto bounds = getLocalBounds();
    const auto baseWidth = proportionOfWidth (0.5f);

    auto makeSquareAndCentreY = [b = std::as_const (bounds)] (auto& rect)
    {
        return rect.withHeight (rect.getWidth()).withCentre ({ rect.getCentreX(), b.getCentreY() });
    };

    const auto undoBounds = bounds.removeFromLeft (baseWidth).withTrimmedLeft (buttonPad).withTrimmedRight (buttonPad / 2);
    undoButton.setBounds (makeSquareAndCentreY (undoBounds));

    const auto redoBounds = bounds.removeFromLeft (baseWidth).withTrimmedRight (buttonPad).withTrimmedLeft (buttonPad / 2);
    redoButton.setBounds (makeSquareAndCentreY (redoBounds));
}
