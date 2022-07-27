#include "CableView.h"
#include "../BoardComponent.h"
#include "CableDrawingHelpers.h"
#include "CableViewConnectionHelper.h"
#include "CableViewPortLocationHelper.h"

CableView::CableView (BoardComponent* comp) : board (comp)
{
    setInterceptsMouseClicks (false, true);
    startTimerHz (36);

    updateCables();
    
    connectionHelper = std::make_unique<CableViewConnectionHelper> (*this);
    portLocationHelper = std::make_unique<CableViewPortLocationHelper> (*this);

}

void CableView::updateCables()
{
    for (auto* cable : cables)
    {
        addChildComponent(cable);
        addAndMakeVisible(cable);
        cable->setBounds(getLocalBounds());
        
    }
    
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
            cable->drawCable (g, startPortLocation.toFloat(), startColour, endPortLocation.toFloat(), endColour, scaleFactor, levelDB);
            cable->repaint();
        }
        else if (connectionHelper->cableMouse != nullptr)
        {
            auto mousePos = connectionHelper->cableMouse->getPosition();
            const auto nearestInputPort = portLocationHelper->getNearestInputPort (mousePos, cable->startProc);
            if (nearestInputPort.editor != nullptr && nearestInputPort.isInput && ! portLocationHelper->isInputPortConnected (nearestInputPort))
            {
                auto endPortLocation = CableViewPortLocationHelper::getPortLocation (nearestInputPort);
                drawCablePortGlow (g, endPortLocation, scaleFactor);
            }

            auto colour = startEditor->getColour();
            cable->drawCable (g, startPortLocation.toFloat(), colour, mousePos.toFloat(), colour, scaleFactor, 0.0f);//this 0 hre dannger
            cable->repaint();
        }
    }
}

void CableView::resized()
{
    for (auto* cable : cables)
    {
        cable->setBounds(getLocalBounds());
    }
    
}

void CableView::mouseDown (const MouseEvent& e)
{
    
    if(e.eventComponent == nullptr)
        return;// not a valid mouse event
    
    if(e.mods.isPopupMenu())
    {
        for(int i = 0; i < cables.size(); ++i)
        {
            Point clicked(e.getMouseDownX(), e.getMouseDownY());
            if(cables[i]->contains(clicked)) //checks hitTest method for cable
            {
                board->popupMenu.showPopupMenu();
                
                board->newCables.add(cables[i]);
                board->generatedFromCableClick = true;
                
                Logger::writeToLog ("CLICK ON PATH " + std::to_string(i));
            }
            else
            {
                Logger::writeToLog ("NOT ON PATH");
            }
            
            
        }
    }
    
    
    if (e.mods.isAnyModifierKeyDown() || e.mods.isPopupMenu() || e.eventComponent == nullptr)
        return; // not a valid mouse event

    nearestPort = portLocationHelper->getNearestPort (e.getEventRelativeTo (this).getMouseDownPosition(), e.source.getComponentUnderMouse());
    if (nearestPort.editor == nullptr)
        return; // no nearest port

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
    updateCables();
}

void CableView::timerCallback()
{
    const auto mousePos = Desktop::getMousePosition() - getScreenPosition();
    nearestPort = portLocationHelper->getNearestPort (mousePos, Desktop::getInstance().getMainMouseSource().getComponentUnderMouse());
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

void CableView::processorBeingAdded (BaseProcessor* newProc, BaseProcessor* inProc, BaseProcessor* outProc, Cable* c)
{
    connectionHelper->processorBeingAdded (newProc, inProc, outProc, c);
}

void CableView::processorBeingRemoved (const BaseProcessor* proc)
{
    connectionHelper->processorBeingRemoved (proc);
}
