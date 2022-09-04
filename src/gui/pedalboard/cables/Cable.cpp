#include "Cable.h"
#include "../BoardComponent.h"
#include "CableDrawingHelpers.h"
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
        if (clickedP.getDistanceFrom (pointOnPath) < cablethickness)
        {
            return true;
        }
    }

    return false;
}

float Cable::getCableThickness()
{
    levelDB = jlimit (floorDB, 0.0f, levelDB);
    auto levelMult = std::pow (jmap (levelDB, floorDB, 0.0f, 0.0f, 1.0f), 0.9f);
    return cableThickness * (1.0f + 0.9f * levelMult);
}

auto Cable::createCablePath (juce::Point<float> start, juce::Point<float> end)
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

void Cable::drawCableShadow (Graphics& g, float thickness)
{
    auto cableShadow = Path (cablePath);
    cableShadow.applyTransform (AffineTransform::translation (0.0f, thickness * 0.6f));
    g.setColour (Colours::black.withAlpha (0.3f));
    g.strokePath (cableShadow, PathStrokeType (cableThickness, PathStrokeType::JointStyle::curved));
}

void Cable::drawCableEndCircle (Graphics& g, juce::Point<float> centre, Colour colour) const
{
    auto circle = (Rectangle { cableThickness, cableThickness } * 2.4f * scaleFactor).withCentre (centre);
    g.setColour (colour);
    g.fillEllipse (circle);

    g.setColour (Colours::white);
    g.drawEllipse (circle, portCircleThickness);
}

void Cable::drawCable (Graphics& g, juce::Point<float> start, juce::Point<float> end)
{
    cablethickness = getCableThickness();
    cablePath = createCablePath (start, end);
    drawCableShadow (g, cablethickness);

    g.setGradientFill (ColourGradient { startColour, start, endColour, end, false });
    g.strokePath (cablePath, PathStrokeType (cablethickness, PathStrokeType::JointStyle::curved));

    drawCableEndCircle (g, start, startColour);
    drawCableEndCircle (g, end, endColour);
}

void Cable::paint (Graphics& g)
{
    g.setColour (cableColour.brighter (0.1f));
    auto* startEditor = board->findEditorForProcessor (connectionInfo.startProc);
    jassert (startEditor != nullptr);

    startPortLocation = CableViewPortLocationHelper::getPortLocation ({ startEditor, connectionInfo.startPort, false }).toFloat();
    scaleFactor = board->getScaleFactor();
    startColour = startEditor->getColour();

    if (connectionInfo.endProc != nullptr)
    {
        auto* endEditor = board->findEditorForProcessor (connectionInfo.endProc);
        jassert (endEditor != nullptr);

        endPortLocation = CableViewPortLocationHelper::getPortLocation ({ endEditor, connectionInfo.endPort, true }).toFloat();
        endColour = endEditor->getColour();
        levelDB = connectionInfo.endProc->getInputLevelDB (connectionInfo.endPort);

        drawCable (g, startPortLocation, endPortLocation);
    }
    else if (cableView.cableBeingDragged())
    {
        endColour = startColour;
        drawCable (g, startPortLocation, cableView.getCableMousePosition());
    }
}
