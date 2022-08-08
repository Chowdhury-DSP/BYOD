#include "CableView.h"
#include "../BoardComponent.h"
#include "CableDrawingHelpers.h"
#include "CableViewConnectionHelper.h"
#include "CableViewPortLocationHelper.h"

CableView::CableView (BoardComponent* comp) : board (comp)
{
    setInterceptsMouseClicks (false, true);
    startTimerHz (36);

    connectionHelper = std::make_unique<CableViewConnectionHelper> (*this);
    portLocationHelper = std::make_unique<CableViewPortLocationHelper> (*this);
}

CableView::~CableView() = default;

void CableView::paint (Graphics& g)
{
    using namespace CableConstants;
    using namespace CableDrawingHelpers;

    if (nearestPort.editor != nullptr)
    {
        const bool isDraggingNearInputPort = nearestPort.isInput && isDraggingCable;
        const bool isNearConnectedInput = nearestPort.isInput && ! portLocationHelper->isInputPortConnected (nearestPort);
        if (! (isDraggingNearInputPort || isNearConnectedInput))
        {
            auto startPortLocation = CableViewPortLocationHelper::getPortLocation (nearestPort);
            drawCablePortGlow (g, startPortLocation, scaleFactor);
        }
    }

    if (cableMouse != nullptr)
    {
        auto mousePos = cableMouse->getPosition();
        const auto nearestInputPort = portLocationHelper->getNearestInputPort (mousePos, cables.getLast()->startProc);
        if (nearestInputPort.editor != nullptr && nearestInputPort.isInput && ! portLocationHelper->isInputPortConnected (nearestInputPort))
        {
            auto endPortLocation = CableViewPortLocationHelper::getPortLocation (nearestInputPort);
            drawCablePortGlow (g, endPortLocation, scaleFactor);
        }
    }
}

void CableView::resized()
{
    for (auto* cable : cables)
    {
        cable->setBounds (getLocalBounds());
    }
}

void CableView::mouseDown (const MouseEvent& e)
{
    if (e.eventComponent == nullptr)
        return; // not a valid mouse event

    if (e.mods.isPopupMenu())
    {
        for (auto* cable : cables)
        {
            juce::Point clicked (e.getMouseDownX(), e.getMouseDownY());
            if (cable->contains (clicked)) // Checks hitTest method for cable
            {
                board->popupMenu.showPopupMenu();
                connectionHelper->clickOnCable (cable);
                return;
            }
        }
    }

    if (e.mods.isAnyModifierKeyDown())
        return; // not a valid mouse event

    nearestPort = portLocationHelper->getNearestPort (e.getEventRelativeTo (this).getMouseDownPosition(), e.source.getComponentUnderMouse());
    if (nearestPort.editor == nullptr)
        return; // no nearest port

    if (nearestPort.isInput)
    {
        connectionHelper->destroyCable (nearestPort.editor->getProcPtr(), nearestPort.portIndex);
    }
    else
    {
        connectionHelper->createCable ({ nearestPort.editor->getProcPtr(), nearestPort.portIndex, nullptr, 0 });
        cableMouse = std::make_unique<MouseEvent> (std::move (e.getEventRelativeTo (this)));
        isDraggingCable = true;
    }
}

void CableView::mouseDrag (const MouseEvent& e)
{
    if (isDraggingCable)
        cableMouse = std::make_unique<MouseEvent> (std::move (e.getEventRelativeTo (this)));
}

void CableView::mouseUp (const MouseEvent& e)
{
    if (isDraggingCable)
    {
        cableMouse.reset();
        connectionHelper->releaseCable (e);
        isDraggingCable = false;
    }
}

void CableView::timerCallback()
{
    const auto mousePos = Desktop::getMousePosition() - getScreenPosition();
    nearestPort = portLocationHelper->getNearestPort (mousePos, Desktop::getInstance().getMainMouseSource().getComponentUnderMouse());
    repaint();
}

void CableView::processorBeingAdded (BaseProcessor* newProc)
{
    connectionHelper->processorBeingAdded (newProc);
}

void CableView::processorBeingRemoved (const BaseProcessor* proc)
{
    connectionHelper->processorBeingRemoved (proc);
}
