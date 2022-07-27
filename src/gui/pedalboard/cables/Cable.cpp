#include "Cable.h"
#include "../BoardComponent.h"
#include "CableDrawingHelpers.h"
#include "CableViewConnectionHelper.h"
#include "CableViewPortLocationHelper.h"


Cable::Cable (BaseProcessor* start, int startPort) : startProc (start),
                                          startIdx (startPort),  cableColour (0xFFD0592C)
{

   
}

Cable::Cable (BaseProcessor* start, int startPort, BaseProcessor* end, int endPort) : startProc (start),
                                                                           startIdx (startPort),
                                                                           endProc (end),
                                                                           endIdx (endPort),  cableColour (0xFFD0592C)
{

}

Cable::~Cable() = default;

float Cable::getCableThickness (float levelDB)
{
    levelDB = jlimit (floorDB, 0.0f, levelDB);
    auto levelMult = std::pow (jmap (levelDB, floorDB, 0.0f, 0.0f, 1.0f), 0.9f);
    return cableThickness * (1.0f + 0.9f * levelMult);
}

auto Cable::createCablePath (Point<float> start, Point<float> end, float scaleFactor)
{
    const auto pointOff = portOffset + scaleFactor;
    bezier = CubicBezier (start, start.translated (pointOff, 0.0f), end.translated (-pointOff, 0.0f), end);
    nump = (int) start.getDistanceFrom (end) + 1;
    //cube = bezier;

    //nump = numPoints;
    Path bezierPath;
    bezierPath.preallocateSpace (nump * 3 / 2);
    bezierPath.startNewSubPath (start);
    for (int i = 1; i <= nump; ++i)
        bezierPath.lineTo (bezier.pointOnCubicBezier ((float) i / (float) nump));
    //bezierPath.closeSubPath();

    return std::move (bezierPath);
}


void Cable::drawCable (Graphics& g, Point<float> start, Colour startColour, Point<float> end, Colour endColour, float scaleFactor, float levelDB)
{
    auto thickness = getCableThickness (levelDB);
    cablethickness = thickness;
    cablePath = createCablePath (start, end, scaleFactor);
    drawCableShadow (g, cablePath, thickness);

    g.setGradientFill (ColourGradient { startColour, start, endColour, end, false });
    g.strokePath (cablePath, PathStrokeType (thickness, PathStrokeType::JointStyle::curved));

    drawCableEndCircle (g, start, startColour, scaleFactor);
    drawCableEndCircle (g, end, endColour, scaleFactor);
    

}

void Cable::drawCableShadow (Graphics& g, const Path& cablePath, float thickness)
{
    auto cableShadow = Path (cablePath);
    cableShadow.applyTransform (AffineTransform::translation (0.0f, thickness * 0.6f));
    g.setColour (Colours::black.withAlpha (0.3f));
    g.strokePath (cableShadow, PathStrokeType (cableThickness, PathStrokeType::JointStyle::curved));
}

void Cable::drawCableEndCircle (Graphics& g, Point<float> centre, Colour colour, float scaleFactor)
{
    auto circle = (Rectangle { cableThickness, cableThickness } * 2.4f * scaleFactor).withCentre (centre);
    g.setColour (colour);
    g.fillEllipse (circle);

    g.setColour (Colours::white);
    g.drawEllipse (circle, portCircleThickness);
}



void Cable::paint (Graphics& g) 
{
    using namespace CableConstants;
    using namespace CableDrawingHelpers;
    g.setColour (juce::Colours::green);
    g.drawLine (0.0f, 0.0f, getWidth(), getHeight());
    auto rec = cablePath.getBounds();
    g.drawRect(rec.getX(), rec.getY(), rec.getWidth(), rec.getHeight());
    // This just here for testing cable bounds and hit boxes
}

bool Cable::hitTest(int x, int y)
{


    Point p((float)x, (float)y );
    Point p2((float)x, (float)y );
    for (int i = 1; i <= nump; ++i)
    {
        auto pointOnPath = bezier.pointOnCubicBezier ((float) i / (float) nump);
        if (p.getDistanceFrom(pointOnPath) < cablethickness)
        {
            return true;
        }
    }
        return false;
}

