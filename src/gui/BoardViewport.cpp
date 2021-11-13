#include "BoardViewport.h"

BoardViewport::BoardViewport (ProcessorChain& procChain) : comp (procChain)
{
    setViewedComponent (&comp, false);

    getHorizontalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    getVerticalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    setScrollBarsShown (true, true);

    const auto buttonColour = Colours::azure.darker (0.8f).withAlpha (0.75f);
    plusButton.setColour (TextButton::buttonColourId, buttonColour);
    addAndMakeVisible (plusButton);
    plusButton.onClick = [=]
    {
        scaleFactor *= 1.1f;
        scaleLabel.setText (String (int (scaleFactor * 100.0f)) + "%", dontSendNotification);
        resized();
    };

    minusButton.setColour (TextButton::buttonColourId, buttonColour);
    addAndMakeVisible (minusButton);
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
    const auto width = getWidth();
    const auto height = getHeight();

    comp.setScaleFactor (scaleFactor);
    comp.setBounds (0, 0, width, height);

    constexpr int buttonDim = 34;
    auto buttonRect = Rectangle { 0, height - buttonDim, buttonDim, buttonDim };
    plusButton.setBounds (buttonRect.reduced (1));
    minusButton.setBounds (buttonRect.withX (buttonDim).reduced (1));
    scaleLabel.setBounds (2 * buttonDim, height - buttonDim, 100, buttonDim);
}
