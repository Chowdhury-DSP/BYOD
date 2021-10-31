#include "BoardViewport.h"

BoardViewport::BoardViewport (ProcessorChain& procChain) : comp (procChain)
{
    setViewedComponent (&comp, false);

    getHorizontalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    getVerticalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    setScrollBarsShown (true, true);

    // addAndMakeVisible (plusButton);
    // addAndMakeVisible (minusButton);

    plusButton.onClick = [=]
    {
        scaleFactor *= 1.5f;
        resized();
    };

    minusButton.onClick = [=]
    {
        scaleFactor /= 1.5f;
        resized();
    };
}

BoardViewport::~BoardViewport()
{
    getHorizontalScrollBar().setLookAndFeel (nullptr);
}

void BoardViewport::resized()
{
    if (scaleFactor == 1.0f)
    {
        comp.setBounds (0, 0, getWidth(), getHeight());
    }
    else if (scaleFactor < 1.0f)
    {
        comp.setBounds (0, 0, getWidth(), getHeight());
        comp.setTransform (AffineTransform::scale (scaleFactor));
    }
    else
    {
        comp.setBounds (0, 0, getWidth(), getHeight());
        comp.setTransform (AffineTransform::scale (scaleFactor));
    }

    plusButton.setBounds (0, getHeight() - 30, 30, 30);
    minusButton.setBounds (30, getHeight() - 30, 30, 30);
}
