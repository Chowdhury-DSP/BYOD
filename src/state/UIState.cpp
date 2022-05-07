#include "UIState.h"
#include "gui/GUIConstants.h"

namespace
{
const Identifier lastSizeTag { "last-size" };
const Identifier widthTag { "ui-width" };
const Identifier heightTag { "ui-height" };
} // namespace

UIState::UIState (AudioProcessorValueTreeState& vtState) : vts (vtState)
{
}

void UIState::attachToComponent (Component& comp)
{
    comp.addComponentListener (this);
}

std::pair<int, int> UIState::getLastEditorSize() const
{
    auto sizeNode = vts.state.getOrCreateChildWithName (lastSizeTag, nullptr);
    if (! sizeNode.hasProperty (widthTag) || ! sizeNode.hasProperty (heightTag))
        return { GUIConstants::defaultWidth, GUIConstants::defaultHeight };

    return { sizeNode.getProperty (widthTag), sizeNode.getProperty (heightTag) };
}

void UIState::componentBeingDeleted (Component& component)
{
    component.removeComponentListener (this);
}

void UIState::componentMovedOrResized (Component& component, bool /*wasMoved*/, bool wasResized)
{
    if (wasResized)
        setLastEditorSize (component.getWidth(), component.getHeight());
}

void UIState::setLastEditorSize (int width, int height)
{
    auto sizeNode = vts.state.getOrCreateChildWithName (lastSizeTag, nullptr);
    sizeNode.setProperty (widthTag, width, nullptr);
    sizeNode.setProperty (heightTag, height, nullptr);
}
