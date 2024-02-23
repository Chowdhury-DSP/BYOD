#include "BoardViewport.h"

namespace ViewportTags
{
const juce::Identifier zoomLevelTag { "zoom_level" };
}

BoardViewport::BoardViewport (AudioProcessorValueTreeState& vts,
                              ProcessorChain& procChain,
                              chowdsp::HostContextProvider& hostContextProvider)
    : comp (procChain, hostContextProvider)
{
    pluginSettings->addProperties<&BoardViewport::globalSettingChanged> ({ { defaultZoomSettingID, 1.0 },
                                                                           { portTooltipsSettingID, true } },
                                                                         *this);
    if (! vts.state.hasProperty (ViewportTags::zoomLevelTag))
        vts.state.setProperty (ViewportTags::zoomLevelTag, pluginSettings->getProperty<double> (defaultZoomSettingID), nullptr);
    scaleFactor = vts.state.getPropertyAsValue (ViewportTags::zoomLevelTag, nullptr, true);
    setScaleFactor ((float) scaleFactor.getValue());

    toggleTooltips (pluginSettings->getProperty<bool> (portTooltipsSettingID));

    setViewedComponent (&comp, false);

    getHorizontalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    getVerticalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    setScrollBarsShown (true, true);

    const auto buttonColour = Colours::azure.darker (0.8f).withAlpha (0.75f);
    auto plusImage = Drawable::createFromImageData (BinaryData::magnifyingglassplussolid_svg, BinaryData::magnifyingglassplussolid_svgSize);
    plusButton.setImages (plusImage.get());
    plusButton.setColour (TextButton::buttonColourId, buttonColour);
    addAndMakeVisible (plusButton);
    plusButton.onClick = [this]
    {
        setScaleFactor ((float) scaleFactor.getValue() * 1.1f);
        resized();
    };

    auto minusImage = Drawable::createFromImageData (BinaryData::magnifyingglassminussolid_svg, BinaryData::magnifyingglassminussolid_svgSize);
    minusButton.setImages (minusImage.get());
    minusButton.setColour (TextButton::buttonColourId, buttonColour);
    addAndMakeVisible (minusButton);
    minusButton.onClick = [this]
    {
        setScaleFactor ((float) scaleFactor.getValue() / 1.1f);
        resized();
    };

    addAndMakeVisible (scaleLabel);
}

void BoardViewport::globalSettingChanged (SettingID settingID)
{
    if (settingID == defaultZoomSettingID)
    {
        setScaleFactor ((float) pluginSettings->getProperty<double> (settingID));
        resized();
        Logger::writeToLog ("Default zoom level set to: " + scaleLabel.getText());
    }
    else if (settingID == portTooltipsSettingID)
    {
        const auto shouldShowTooltips = pluginSettings->getProperty<bool> (portTooltipsSettingID);
        Logger::writeToLog ("Showing port tooltips: " + String (shouldShowTooltips ? "TRUE" : "FALSE"));
        toggleTooltips (shouldShowTooltips);
    }
}

void BoardViewport::setScaleFactor (float newScaleFactor)
{
    if (newScaleFactor < 0.45f || newScaleFactor > 1.8f)
        return; // limits for zoom level

    scaleFactor = newScaleFactor;
    scaleLabel.setText (String (int ((float) scaleFactor.getValue() * 100.0f)) + "%", dontSendNotification);
}

void BoardViewport::toggleTooltips (bool shouldShow)
{
    if (shouldShow && ! tooltips.has_value())
        tooltips.emplace (this, 1000);
    else if (! shouldShow && tooltips.has_value())
        tooltips.reset();
}

void BoardViewport::resized()
{
    TRACE_COMPONENT();

    const auto width = getWidth();
    const auto height = getHeight();

    comp.setScaleFactor ((float) scaleFactor.getValue());
    comp.setBounds (0, 0, width, height);

    constexpr int buttonDim = 34;
    auto buttonRect = juce::Rectangle { 0, height - buttonDim, buttonDim, buttonDim };
    plusButton.setBounds (buttonRect.reduced (1));
    minusButton.setBounds (buttonRect.withX (buttonDim).reduced (1));
    scaleLabel.setBounds (2 * buttonDim, height - buttonDim, 100, buttonDim);
}
