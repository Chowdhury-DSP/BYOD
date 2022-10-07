#include "Cable.h"
#include "../BoardComponent.h"
#include "CableViewConnectionHelper.h"
#include "CableViewPortLocationHelper.h"

using namespace CableConstants;

Cable::Cable (const BoardComponent* comp, CableView& cv, const ConnectionInfo connection) : Component (Cable::componentName.data()),
                                                                                            connectionInfo (connection),
                                                                                            cableView (cv),
                                                                                            board (comp)
{
    popupMenu.setAssociatedComponent (this);
    popupMenu.popupMenuCallback = [&] (PopupMenu& menu, PopupMenu::Options& options)
    { cableView.getConnectionHelper()->clickOnCable (menu, options, this); };
}

Cable::~Cable() = default;

bool Cable::hitTest (int x, int y)
{
    juce::Point clickedP ((float) x, (float) y);
    for (int i = 1; i <= numPointsInPath; ++i)
    {
        auto pointOnPath = bezier.getPointOnCubicBezier ((float) i / (float) numPointsInPath);
        if (clickedP.getDistanceFrom (pointOnPath) < cableThickness)
        {
            return true;
        }
    }

    return false;
}

void Cable::updateStartPoint()
{
    auto* startEditor = board->findEditorForProcessor (connectionInfo.startProc);
    jassert (startEditor != nullptr);

    startPoint = CableViewPortLocationHelper::getPortLocation ({ startEditor, connectionInfo.startPort, false }).toFloat();
    scaleFactor = board->getScaleFactor();
    startColour = startEditor->getColour();
    cableThickness = getCableThickness();
}

void Cable::updateEndPoint()
{
    if (connectionInfo.endProc != nullptr)
    {
        auto* endEditor = board->findEditorForProcessor (connectionInfo.endProc);
        endPoint = CableViewPortLocationHelper::getPortLocation ({ endEditor, connectionInfo.endPort, true }).toFloat();
        endColour = endEditor->getColour();
    }
    else if (cableView.cableBeingDragged())
    {
        endColour = startColour;
        endPoint = cableView.getCableMousePosition();
    }
}

Path Cable::createCablePath (juce::Point<float> start, juce::Point<float> end, float scaleFactor)
{
    const auto pointOff = portOffset + scaleFactor;
    bezier = CubicBezier (start, start.translated (pointOff, 0.0f), end.translated (-pointOff, 0.0f), end);
    numPointsInPath = (int) start.getDistanceFrom (end) + 1;
    Path bezierPath;
    bezierPath.preallocateSpace (numPointsInPath * 3 / 2);
    bezierPath.startNewSubPath (start);
    for (int i = 1; i <= numPointsInPath; ++i)
        bezierPath.lineTo (bezier.getPointOnCubicBezier ((float) i / (float) numPointsInPath));

    return std::move (bezierPath);
}

void Cable::checkNeedsRepaint()
{
    auto createdPath = createCablePath (startPoint, endPoint, scaleFactor);
    {
        ScopedLock sl (pathCrit);
        cablePath = std::move (createdPath);
    }

    if (connectionInfo.endProc != nullptr)
    {
        auto updatedLevelDB = jlimit (floorDB, 0.0f, connectionInfo.endProc->getInputLevelDB (connectionInfo.endPort));
        auto levelDifference = std::abs (updatedLevelDB - levelDB);
        if (std::abs (levelDifference) > 2.0f && levelRange.contains (floorDB))
        {
            levelDB = updatedLevelDB;
            cableBounds = cablePath.getBounds().toNearestInt();
            cableBounds.setY (cableBounds.getY() - roundToInt (std::ceil (minCableThickness)));
            cableBounds.setHeight (cableBounds.getHeight() + roundToInt (std::ceil (2.0f * minCableThickness)));
            MessageManager::callAsync ([&]
                                       { repaint (cableBounds); });
        }
    }
}

float Cable::getCableThickness()
{
    auto levelMult = std::pow (jmap (levelDB, floorDB, 0.0f, 0.0f, 1.0f), 0.9f);
    return minCableThickness * (1.0f + 0.9f * levelMult);
}

void Cable::drawCableShadow (Graphics& g, float thickness)
{
    auto cableShadow = Path (cablePath);
    cableShadow.applyTransform (AffineTransform::translation (0.0f, thickness * 0.6f));
    g.setColour (Colours::black.withAlpha (0.3f));
    g.strokePath (cableShadow, PathStrokeType (minCableThickness, PathStrokeType::JointStyle::curved));
}

void Cable::drawCableEndCircle (Graphics& g, juce::Point<float> centre, Colour colour) const
{
    auto circle = (Rectangle { minCableThickness, minCableThickness } * 2.4f * scaleFactor.load()).withCentre (centre);
    g.setColour (colour);
    g.fillEllipse (circle);

    g.setColour (Colours::white);
    g.drawEllipse (circle, portCircleThickness);
}

void Cable::drawCable (Graphics& g, juce::Point<float> start, juce::Point<float> end)
{
    drawCableShadow (g, cableThickness);
    g.setGradientFill (ColourGradient { startColour, start, endColour, end, false });

    {
        ScopedLock sl (pathCrit);
        g.strokePath (cablePath, PathStrokeType (cableThickness, PathStrokeType::JointStyle::curved));
    }

    drawCableEndCircle (g, start, startColour);
    drawCableEndCircle (g, end, endColour);
}

void Cable::paint (Graphics& g)
{
    g.setColour (cableColour.brighter (0.1f));

    updateStartPoint();
    updateEndPoint();

    drawCable (g, startPoint, endPoint);
}
