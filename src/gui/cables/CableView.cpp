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

    if (outputPortToHighlight.first != nullptr)
    {
        auto startPortLocation = CableViewPortLocationHelper::getPortLocation (outputPortToHighlight.first, outputPortToHighlight.second, false);
        drawCablePortGlow (g, startPortLocation);
    }

    g.setColour (cableColour.brighter (0.1f));
    for (auto* cable : cables)
    {
        auto* startEditor = board->findEditorForProcessor (cable->startProc);
        jassert (startEditor != nullptr);

        auto startPortLocation = CableViewPortLocationHelper::getPortLocation (startEditor, cable->startIdx, false);

        if (cable->endProc != nullptr)
        {
            auto* endEditor = board->findEditorForProcessor (cable->endProc);
            jassert (endEditor != nullptr);

            auto endPortLocation = CableViewPortLocationHelper::getPortLocation (endEditor, cable->endIdx, true);

            auto startColour = startEditor->getColour();
            auto endColour = endEditor->getColour();
            auto levelDB = cable->endProc->getInputLevelDB (cable->endIdx);
            drawCable (g, startPortLocation.toFloat(), startColour, endPortLocation.toFloat(), endColour, scaleFactor, levelDB);
        }
        else if (connectionHelper->cableMouse != nullptr)
        {
            auto mousePos = connectionHelper->cableMouse->getPosition();
            auto [editor, portIdx] = portLocationHelper->getNearestInputPort (mousePos, cable->startProc);
            if (editor != nullptr)
            {
                auto endPortLocation = CableViewPortLocationHelper::getPortLocation (editor, portIdx, true);
                drawCablePortGlow (g, endPortLocation);
            }

            auto colour = startEditor->getColour();
            drawCable (g, startPortLocation.toFloat(), colour, mousePos.toFloat(), colour, scaleFactor);
        }
    }
}

void CableView::timerCallback()
{
    const auto mousePos = Desktop::getMousePosition() - getScreenPosition();
    outputPortToHighlight = portLocationHelper->getNearestOutputPort (mousePos);
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
