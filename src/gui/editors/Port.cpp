#include "Port.h"
#include "../cables/CableDrawingHelpers.h"

void Port::paint (Graphics& g)
{
    auto portBounds = getLocalBounds().toFloat();
    g.setColour (isInput ? Colours::black : Colours::darkgrey);

    if (isConnected)
    {
        const auto width = (float) getWidth();
        const auto xOff = isInput ? -1.0f : 1.0f;
        const auto bigArcStart = 0.0f;
        const auto bigArcEnd = isInput ? MathConstants<float>::pi + 0.1f : -MathConstants<float>::pi - 0.1f;
        const auto smallArcStart = isInput ? MathConstants<float>::pi : -MathConstants<float>::pi;
        const auto smallArcEnd = isInput ? -0.1f : 0.1f;

        Path arcPath;
        arcPath.startNewSubPath (portBounds.getCentreX() + xOff, portBounds.getY());
        arcPath.addArc (portBounds.getX(), portBounds.getY(), portBounds.getWidth(), portBounds.getHeight(), bigArcStart, bigArcEnd);
        auto pb = portBounds.reduced (width * 0.31f).translated (0.0f, -0.5f);
        arcPath.lineTo (pb.getCentreX() + xOff, pb.getBottom());
        arcPath.addArc (pb.getX(), pb.getY(), pb.getWidth(), pb.getHeight(), smallArcStart, smallArcEnd);
        arcPath.closeSubPath();
        g.fillPath (arcPath);

        g.setColour (Colours::white);
        g.drawEllipse (pb, CableConstants::portCircleThickness);
    }
    else
    {
        g.fillEllipse (portBounds);
    }

    g.setColour (Colours::lightgrey);
    g.drawEllipse (portBounds.reduced (1.0f), 1.0f);
}

void Port::setConnectionStatus (bool connectionStatus)
{
    isConnected = connectionStatus;
    repaint();
}
