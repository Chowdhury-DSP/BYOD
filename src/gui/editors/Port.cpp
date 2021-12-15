#include "Port.h"
#include "../cables/CableDrawingHelpers.h"

void Port::paint (Graphics& g)
{
    auto portBounds = getLocalBounds().toFloat();
    g.setColour (isInput ? Colours::black : Colours::darkgrey);

    if (isConnected)
    {
        const auto width = (float) getWidth();

        Path arcPath;
        arcPath.startNewSubPath (portBounds.getCentreX() - 1.0f, portBounds.getY());
        arcPath.addArc (portBounds.getX(), portBounds.getY(), portBounds.getWidth(), portBounds.getHeight(), 0.0f, MathConstants<float>::pi + 0.1f);
        auto pb = portBounds.reduced (width * 0.31f);
        arcPath.lineTo (pb.getCentreX() - 1.0f, pb.getBottom());
        arcPath.addArc (pb.getX(), pb.getY(), pb.getWidth(), pb.getHeight(), MathConstants<float>::pi, -0.1f);
        arcPath.closeSubPath();
        g.fillPath (arcPath);

        g.setColour (Colours::white);
        g.drawEllipse (pb, 1.0f);
    }
    else
    {
        g.fillEllipse (portBounds);
    }

    g.setColour (Colours::lightgrey);
    g.drawEllipse (portBounds.reduced (1.0f), 1.0f);
}

void Port::mouseDown (const MouseEvent& e)
{
    if (isInput)
        portListeners.call (&PortListener::destroyCable, this);
    else
        portListeners.call (&PortListener::createCable, this, e);
}

void Port::mouseDrag (const MouseEvent& e)
{
    if (! isInput)
        portListeners.call (&PortListener::refreshCable, e);
}

void Port::mouseUp (const MouseEvent& e)
{
    if (! isInput)
        portListeners.call (&PortListener::releaseCable, e);
}

void Port::setConnectionStatus (bool connectionStatus)
{
    isConnected = connectionStatus;
    repaint();
}
