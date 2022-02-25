#include "CableView.h"
#include "../BoardComponent.h"
#include "CableDrawingHelpers.h"
#include "CableViewConnectionHelper.h"
#include "CableViewPortLocationHelper.h"

CableView::CableView (const BoardComponent* comp) : board (comp)
{
    setInterceptsMouseClicks (false, false);
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
        if (! nearestPort.isInput || portLocationHelper->isInputPortConnected (nearestPort))
        {
            auto startPortLocation = CableViewPortLocationHelper::getPortLocation (nearestPort);
            drawCablePortGlow (g, startPortLocation, scaleFactor);
        }
    }

    g.setColour (cableColour.brighter (0.1f));
    for (auto* cable : cables)
    {
        auto* startEditor = board->findEditorForProcessor (cable->startProc);
        jassert (startEditor != nullptr);

        auto startPortLocation = CableViewPortLocationHelper::getPortLocation ({ startEditor, cable->startIdx, false });

        if (cable->endProc != nullptr)
        {
            auto* endEditor = board->findEditorForProcessor (cable->endProc);
            jassert (endEditor != nullptr);

            auto endPortLocation = CableViewPortLocationHelper::getPortLocation ({ endEditor, cable->endIdx, true });

            auto startColour = startEditor->getColour();
            auto endColour = endEditor->getColour();
            auto levelDB = cable->endProc->getInputLevelDB (cable->endIdx);
            drawCable (g, startPortLocation.toFloat(), startColour, endPortLocation.toFloat(), endColour, scaleFactor, levelDB);
        }
        else if (connectionHelper->cableMouse != nullptr)
        {
            auto mousePos = connectionHelper->cableMouse->getPosition();
            if (nearestPort.editor != nullptr && nearestPort.isInput)
            {
                auto endPortLocation = CableViewPortLocationHelper::getPortLocation (nearestPort);
                drawCablePortGlow (g, endPortLocation, scaleFactor);
            }

            auto colour = startEditor->getColour();
            drawCable (g, startPortLocation.toFloat(), colour, mousePos.toFloat(), colour, scaleFactor);
        }
    }
}

void CableView::mouseDown (const MouseEvent& e)
{
    nearestPort = portLocationHelper->getNearestPort (e.getEventRelativeTo (this).getMouseDownPosition());
    if (e.mods.isAnyModifierKeyDown() || e.mods.isPopupMenu() || nearestPort.editor == nullptr)
        return;

    if (nearestPort.isInput)
    {
        connectionHelper->destroyCable (nearestPort.editor, nearestPort.portIndex);
    }
    else
    {
        connectionHelper->createCable (nearestPort.editor, nearestPort.portIndex, e);
        isDraggingCable = true;
    }
}

void CableView::mouseDrag (const MouseEvent& e)
{
    if (isDraggingCable)
        connectionHelper->refreshCable (e);
}

void CableView::mouseUp (const MouseEvent& e)
{
    if (isDraggingCable)
    {
        connectionHelper->releaseCable (e);
        isDraggingCable = false;
    }
}

void CableView::timerCallback()
{
    const auto mousePos = Desktop::getMousePosition() - getScreenPosition();
    nearestPort = portLocationHelper->getNearestPort (mousePos);
    repaint();
}

void CableView::setScaleFactor (float newScaleFactor)
{
    scaleFactor = newScaleFactor;
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
