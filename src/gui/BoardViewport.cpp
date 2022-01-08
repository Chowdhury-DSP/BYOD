#include "BoardViewport.h"

BoardViewport::BoardViewport (ProcessorChain& procChain) : comp (procChain)
{
    pluginSettings->addProperties ({ { defaultZoomSettingID, 1.0 } }, this);
    setScaleFactor ((float) (double) pluginSettings->getProperty (defaultZoomSettingID));

    setViewedComponent (&comp, false);

    getHorizontalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    getVerticalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    setScrollBarsShown (true, true);

    const auto buttonColour = Colours::azure.darker (0.8f).withAlpha (0.75f);
    plusButton.setColour (TextButton::buttonColourId, buttonColour);
    addAndMakeVisible (plusButton);
    plusButton.onClick = [=]
    {
        setScaleFactor (scaleFactor * 1.1f);
        resized();
    };

    minusButton.setColour (TextButton::buttonColourId, buttonColour);
    addAndMakeVisible (minusButton);
    minusButton.onClick = [=]
    {
        setScaleFactor (scaleFactor / 1.1f);
        resized();
    };

    addAndMakeVisible (scaleLabel);
}

BoardViewport::~BoardViewport()
{
    pluginSettings->removePropertyListener (this);
    getHorizontalScrollBar().setLookAndFeel (nullptr);
}

void BoardViewport::propertyChanged (const Identifier& settingID, const var& property)
{
    if (settingID != defaultZoomSettingID)
        return;

    if (! property.isDouble())
        return;

    setScaleFactor ((float) (double) property);
    resized();
}

void BoardViewport::setScaleFactor (float newScaleFactor)
{
    scaleFactor = newScaleFactor;
    scaleLabel.setText (String (int (scaleFactor * 100.0f)) + "%", dontSendNotification);
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

const Identifier BoardViewport::defaultZoomSettingID { "default_zoom" };
