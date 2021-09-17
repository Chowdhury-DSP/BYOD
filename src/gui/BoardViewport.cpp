#include "BoardViewport.h"

BoardViewport::BoardViewport (ProcessorChain& procChain) : comp (procChain)
{
    setViewedComponent (&comp, false);

    getHorizontalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    setScrollBarsShown (false, true);
}

BoardViewport::~BoardViewport()
{
    getHorizontalScrollBar().setLookAndFeel (nullptr);
}

void BoardViewport::resized()
{
    comp.setBounds (0, 0, comp.getIdealWidth (getWidth()), getHeight());
    setScrollBarThickness (BoardComponent::yOffset * 2);
}
