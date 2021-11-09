#include "BoardViewport.h"

BoardViewport::BoardViewport (ProcessorChain& procChain) : comp (procChain)
{
    setViewedComponent (&comp, false);

    getHorizontalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    getVerticalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    setScrollBarsShown (true, true);

    addAndMakeVisible (plusButton);
    addAndMakeVisible (minusButton);

    plusButton.onClick = [=]
    {
        scaleFactor *= 1.1f;
        scaleLabel.setText (String (int (scaleFactor * 100.0f)) + "%", dontSendNotification);
        resized();
    };

    minusButton.onClick = [=]
    {
        scaleFactor /= 1.1f;
        scaleLabel.setText (String (int (scaleFactor * 100.0f)) + "%", dontSendNotification);
        resized();
    };

    scaleLabel.setText (String (int (scaleFactor * 100.0f)) + "%", dontSendNotification);
    addAndMakeVisible (scaleLabel);
}

BoardViewport::~BoardViewport()
{
    getHorizontalScrollBar().setLookAndFeel (nullptr);
}

void BoardViewport::resized()
{
    comp.setScaleFactor (scaleFactor);
    comp.setBounds (0, 0, getWidth(), getHeight());

    plusButton.setBounds (0, getHeight() - 30, 30, 30);
    minusButton.setBounds (30, getHeight() - 30, 30, 30);
    scaleLabel.setBounds (60, getHeight() - 30, 100, 30);
}
