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
    popupMenu.popupMenuCallback = [&] (PopupMenu& menu, PopupMenu::Options& options, juce::Point<int> mousePos)
    { cableView.getConnectionHelper()->clickOnCable (menu, options, mousePos, this); };
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

void Cable::updateStartPoint (bool repaintIfMoved)
{
    auto* startEditor = board->findEditorForProcessor (connectionInfo.startProc);
    jassert (startEditor != nullptr);

    scaleFactor = board->getScaleFactor();
    startColour = startEditor->getColour();
    cableThickness = getCableThickness();

    const auto newPortLocation = CableViewPortLocationHelper::getPortLocation ({ startEditor, connectionInfo.startPort, false }).toFloat();
    if (startPoint.load() != newPortLocation)
    {
        startPoint.store (newPortLocation);
        if (repaintIfMoved)
            repaintIfNeeded (true);
    }
}

void Cable::updateEndPoint (bool repaintIfMoved)
{
    if (connectionInfo.endProc != nullptr)
    {
        auto* endEditor = board->findEditorForProcessor (connectionInfo.endProc);
        endColour = endEditor->getColour();

        const auto newPortLocation = CableViewPortLocationHelper::getPortLocation ({ endEditor, connectionInfo.endPort, true }).toFloat();
        if (endPoint.load() != newPortLocation)
        {
            endPoint.store (newPortLocation);
            if (repaintIfMoved)
                repaintIfNeeded (true);
        }
    }
    else if (cableView.cableBeingDragged())
    {
        endColour = startColour;
        endPoint = cableView.getCableMousePosition();
    }
    else
    {
        // when the cable has just been created, we don't have the mouse drag
        // source yet, so let's just set the end point equal to the start point.
        endPoint.store (startPoint);
    }
}

Path Cable::createCablePath (juce::Point<float> start, juce::Point<float> end, float sf)
{
    const auto pointOff = portOffset + sf;
    bezier = CubicBezier (start, start.translated (pointOff, 0.0f), end.translated (-pointOff, 0.0f), end);
    numPointsInPath = (int) start.getDistanceFrom (end) + 1;
    Path bezierPath;
    bezierPath.preallocateSpace ((numPointsInPath + 1) * 3);
    bezierPath.startNewSubPath (start);
    for (int i = 1; i < numPointsInPath; ++i)
    {
        const auto nextPoint = bezier.getPointOnCubicBezier ((float) i / (float) numPointsInPath);
        bezierPath.lineTo (nextPoint);
    }
    bezierPath.lineTo (end);

    return std::move (bezierPath);
}

void Cable::repaintIfNeeded (bool force)
{
    const auto regeneratePath = [this]
    {
        auto createdPath = createCablePath (startPoint, endPoint, scaleFactor);
        ScopedLock sl (pathCrit);
        cablePath = std::move (createdPath);

        const auto cableBounds = cablePath.getBounds().expanded (std::ceil (minCableThickness), std::ceil (2.0f * minCableThickness)).toNearestInt();
        MessageManager::callAsync (
            [safeComp = Component::SafePointer (this), cableBounds]
            {
                if (auto* comp = safeComp.getComponent())
                    comp->repaint (cableBounds);
            });
    };

    if (force || connectionInfo.endProc == nullptr)
    {
        regeneratePath();
        return;
    }

    if (connectionInfo.endProc != nullptr)
    {
        auto updatedLevelDB = jlimit (floorDB, 0.0f, connectionInfo.endProc->getInputLevelDB (connectionInfo.endPort));
        auto levelDifference = std::abs (updatedLevelDB - levelDB);
        if (std::abs (levelDifference) > 2.0f && levelRange.contains (updatedLevelDB) && levelRange.contains (levelDB))
        {
            levelDB = updatedLevelDB;
            cableThickness = getCableThickness();
            regeneratePath();
        }
    }
}

float Cable::getCableThickness() const
{
    auto levelMult = std::pow (jmap (levelDB, floorDB, 0.0f, 0.0f, 1.0f), 0.9f);
    return minCableThickness * (1.0f + 0.9f * levelMult);
}

void Cable::drawCableShadow (Graphics& g, float thickness)
{
    auto cableShadow = [this]
    {
        ScopedLock sl (pathCrit);
        return Path { cablePath };
    }();
    cableShadow.applyTransform (AffineTransform::translation (0.0f, thickness * 0.6f));
    g.setColour (Colours::black.withAlpha (0.3f));
    g.strokePath (cableShadow, PathStrokeType (minCableThickness, PathStrokeType::JointStyle::curved));
}

void Cable::drawCableEndCircle (Graphics& g, juce::Point<float> centre, Colour colour) const
{
    auto circle = (juce::Rectangle { minCableThickness, minCableThickness } * 2.4f * scaleFactor.load()).withCentre (centre);
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
    drawCable (g, startPoint, endPoint);
}

void Cable::resized()
{
    updateStartPoint (false);
    updateEndPoint (false);
    repaintIfNeeded (true);
}
