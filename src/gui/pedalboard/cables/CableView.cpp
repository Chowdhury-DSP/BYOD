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

bool CableView::mouseOverClickablePort()
{
    const auto mousePos = Desktop::getMousePosition() - getScreenPosition();
    nearestPort = portLocationHelper->getNearestPort (mousePos, board);
    if (nearestPort.editor != nullptr)
    {
        const bool isDraggingNearInputPort = nearestPort.isInput && isDraggingCable;
        const bool isNearConnectedInput = nearestPort.isInput && ! portLocationHelper->isInputPortConnected (nearestPort);
        if (! (isDraggingNearInputPort || isNearConnectedInput))
        {
            startPortLocation = CableViewPortLocationHelper::getPortLocation (nearestPort);
            portToPaint = startPortLocation;
            return true;
        }
    }

    return false;
}

bool CableView::mouseDraggingOverOutputPort()
{
    if (cableMouse != nullptr)
    {
        auto mousePos = cableMouse->getPosition();
        const auto nearestInputPort = portLocationHelper->getNearestInputPort (mousePos, cables.getLast()->connectionInfo.startProc);
        if (nearestInputPort.editor != nullptr && nearestInputPort.isInput && ! portLocationHelper->isInputPortConnected (nearestInputPort))
        {
            endPortLocation = CableViewPortLocationHelper::getPortLocation (nearestInputPort);
            portToPaint = endPortLocation;
            return true;
        }
    }

    return false;
}

void CableView::paint (Graphics& g)
{
    using namespace CableDrawingHelpers;

    if (portGlow)
    {
        drawCablePortGlow (g, portToPaint, scaleFactor);
    }
}

void CableView::resized()
{
    for (auto* cable : cables)
        cable->setBounds (getLocalBounds());
}

void CableView::mouseDown (const MouseEvent& e)
{
    if (e.mods.isAnyModifierKeyDown() || e.mods.isPopupMenu() || e.eventComponent == nullptr)
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
        cableMouse = std::make_unique<MouseEvent> (e.getEventRelativeTo (this));
        isDraggingCable = true;
    }
}

void CableView::mouseDrag (const MouseEvent& e)
{
    if (isDraggingCable)
        cableMouse = std::make_unique<MouseEvent> (e.getEventRelativeTo (this));
}

bool CableView::cableBeingDragged() const
{
    return isDraggingCable;
}

juce::Point<float> CableView::getCableMousePosition() const
{
    return cableMouse->getPosition().toFloat();
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
    using namespace CableDrawingHelpers;

    // repaint port glow
    if (mouseOverClickablePort() || mouseDraggingOverOutputPort())
    {
        portGlow = true;
        repaint (getPortGlowBounds (portToPaint, scaleFactor).toNearestInt());
    }
    else if (! mouseOverClickablePort() && portGlow)
    {
        portGlow = false;
        repaint (getPortGlowBounds (portToPaint, scaleFactor).toNearestInt());
    }

    // repaint cable(s)
    if (cableBeingDragged())
    {
        cables.getLast()->repaint();
    }
    else
    {
        for (auto* cable : cables)
            cable->checkNeedsRepaint();
    }
}

void CableView::processorBeingAdded (BaseProcessor* newProc)
{
    connectionHelper->processorBeingAdded (newProc);
}

void CableView::processorBeingRemoved (const BaseProcessor* proc)
{
    connectionHelper->processorBeingRemoved (proc);
}
