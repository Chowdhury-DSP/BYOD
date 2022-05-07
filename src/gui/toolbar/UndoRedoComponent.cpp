#include "UndoRedoComponent.h"

namespace
{
constexpr int buttonPad = 10;
}

UndoRedoComponent::UndoRedoComponent (UndoManager& um) : undoManager (um)
{
    undoButton.setImages (Drawable::createFromImageData (BinaryData::undosolid_png, BinaryData::undosolid_pngSize).get());
    addAndMakeVisible (undoButton);
    undoButton.onClick = [this]
    {
        undoManager.undo();
        updateButtonEnablementStates();
    };

    redoButton.setImages (Drawable::createFromImageData (BinaryData::redosolid_png, BinaryData::redosolid_pngSize).get());
    addAndMakeVisible (redoButton);
    redoButton.onClick = [this]
    {
        undoManager.redo();
        updateButtonEnablementStates();
    };

    undoManager.addChangeListener (this);
    updateButtonEnablementStates();
}

UndoRedoComponent::~UndoRedoComponent()
{
    undoManager.removeChangeListener (this);
}

void UndoRedoComponent::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source != &undoManager)
    {
        jassertfalse; //unknown source!
        return;
    }

    updateButtonEnablementStates();
}

void UndoRedoComponent::updateButtonEnablementStates()
{
    undoButton.setEnabled (undoManager.canUndo());
    redoButton.setEnabled (undoManager.canRedo());
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
