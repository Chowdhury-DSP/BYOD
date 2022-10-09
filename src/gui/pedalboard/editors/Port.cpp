#include "Port.h"
#include "../cables/CableDrawingHelpers.h"

Port::Port (const Colour& processorColour, const PortType type) : Component ("Port"),
                                                                  procColour (processorColour),
                                                                  portType (type)
{
}

Colour Port::getPortColour() const
{
    if (portType == PortType::modulation)
    {
        return Colours::rebeccapurple;
    }
    else if (isInput)
    {
        return Colours::black;
    }
    else // Output Port
    {
        return Colours::darkgrey;
    }
}

void Port::paint (Graphics& g)
{
    const auto portBounds = getLocalBounds().toFloat();

    if (isConnected)
    {
        g.setColour (procColour);
        g.fillEllipse (portBounds);

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

        g.setColour (getPortColour());
        g.fillPath (arcPath);

        g.setColour (Colours::white);
        g.drawEllipse (pb, CableConstants::portCircleThickness);
    }
    else
    {
        g.setColour (getPortColour());
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
