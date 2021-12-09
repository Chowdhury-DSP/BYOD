#include "Port.h"
#include "../cables/CableDrawingHelpers.h"

void Port::paint (Graphics& g)
{
    auto portBounds = getLocalBounds().toFloat();
    g.setColour (isInput ? Colours::black : Colours::darkgrey);
    g.fillEllipse (portBounds);

    if (isInput && isConnected)
    {
        g.setColour (CableConstants::cableColour.withAlpha (0.5f));
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
