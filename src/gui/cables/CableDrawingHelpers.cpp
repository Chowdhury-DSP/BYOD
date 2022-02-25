#include "CableDrawingHelpers.h"

namespace CableDrawingHelpers
{
/*
    The logic here was borrowed from TAL Noisemaker,
    as permitted unger GPLv2. Original source code
    is here: https://github.com/DISTRHO/DISTRHO-Ports/blob/master/ports-legacy/tal-noisemaker/source/EnvelopeEditor/SplineUtility.h

    cp is a 4 element array where:
    cp[0] is the starting point, or P0 in the above diagram
    cp[1] is the first control point, or P1 in the above diagram
    cp[2] is the second control point, or P2 in the above diagram
    cp[3] is the end point, or P3 in the above diagram
    t is the parameter value, 0 <= t <= 1
*/
struct CubicBezier
{
    CubicBezier (juce::Point<float> p1, juce::Point<float> p2, juce::Point<float> p3, juce::Point<float> p4)
    {
        /* calculate the polynomial coefficients */
        cx = 3.0f * (p2.getX() - p1.getX());
        bx = 3.0f * (p3.getX() - p2.getX()) - cx;
        ax = p4.getX() - p1.getX() - cx - bx;
        p1x = p1.getX();

        cy = 3.0f * (p2.getY() - p1.getY());
        by = 3.0f * (p3.getY() - p2.getY()) - cy;
        ay = p4.getY() - p1.getY() - cy - by;
        p1y = p1.getY();
    }

    juce::Point<float> pointOnCubicBezier (float t)
    {
        using namespace chowdsp::Polynomials;

        /* calculate the curve point at parameter value t */
        auto xVal = estrin<3> ({ ax, bx, cx, p1x }, t);
        auto yVal = estrin<3> ({ ay, by, cy, p1y }, t);

        return juce::Point { xVal, yVal };
    }

    float ax, bx, cx, p1x;
    float ay, by, cy, p1y;
};

using namespace CableConstants;

float getCableThickness (float levelDB)
{
    levelDB = jlimit (floorDB, 0.0f, levelDB);
    auto levelMult = std::pow (jmap (levelDB, floorDB, 0.0f, 0.0f, 1.0f), 0.9f);
    return cableThickness * (1.0f + 0.9f * levelMult);
}

auto createCablePath (Point<float> start, Point<float> end, float scaleFactor)
{
    const auto pointOff = portOffset + scaleFactor;
    auto bezier = CubicBezier (start, start.translated (pointOff, 0.0f), end.translated (-pointOff, 0.0f), end);
    auto numPoints = (int) start.getDistanceFrom (end) + 1;

    Path bezierPath;
    bezierPath.preallocateSpace (numPoints * 3 / 2);
    bezierPath.startNewSubPath (start);
    for (int i = 1; i <= numPoints; ++i)
        bezierPath.lineTo (bezier.pointOnCubicBezier ((float) i / (float) numPoints));

    return std::move (bezierPath);
}

void drawCableShadow (Graphics& g, const Path& cablePath, float thickness)
{
    auto cableShadow = Path (cablePath);
    cableShadow.applyTransform (AffineTransform::translation (0.0f, thickness * 0.6f));
    g.setColour (Colours::black.withAlpha (0.3f));
    g.strokePath (cableShadow, PathStrokeType (cableThickness, PathStrokeType::JointStyle::curved));
}

void drawCableEndCircle (Graphics& g, Point<float> centre, Colour colour, float scaleFactor)
{
    auto circle = (Rectangle { cableThickness * 2.0f, cableThickness * 2.0f } * scaleFactor).withCentre (centre);
    g.setColour (colour);
    g.fillEllipse (circle);

    g.setColour (Colours::white);
    g.drawEllipse (circle, portCircleThickness);
}

void drawCable (Graphics& g, Point<float> start, Colour startColour, Point<float> end, Colour endColour, float scaleFactor, float levelDB)
{
    auto thickness = getCableThickness (levelDB);
    auto cablePath = createCablePath (start, end, scaleFactor);
    drawCableShadow (g, cablePath, thickness);

    g.setGradientFill (ColourGradient { startColour, start, endColour, end, false });
    g.strokePath (cablePath, PathStrokeType (thickness, PathStrokeType::JointStyle::curved));

    drawCableEndCircle (g, start, startColour, scaleFactor);
    drawCableEndCircle (g, end, endColour, scaleFactor);
}

void drawCablePortGlow (Graphics& g, Point<int> location)
{
    Graphics::ScopedSaveState graphicsState (g);
    g.setColour (cableColour.darker (0.1f));
    g.setOpacity (0.65f);

    auto glowBounds = (Rectangle (portDistanceLimit, portDistanceLimit).toFloat() * 2.5f).withCentre (location.toFloat());
    g.fillEllipse (glowBounds);
}
} // namespace CableDrawingHelpers
