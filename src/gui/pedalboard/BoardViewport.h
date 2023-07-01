#pragma once

#include "BYOD.h"
#include "BoardComponent.h"

class BoardViewport : public Viewport,
                      public chowdsp::TrackedByBroadcasters
{
public:
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;

    BoardViewport (AudioProcessorValueTreeState& vts, ProcessorChain& procChain, chowdsp::HostContextProvider& hostContextProvider);

    void resized() override;

    void globalSettingChanged (SettingID settingID);

    static constexpr SettingID defaultZoomSettingID = "default_zoom";
    static constexpr SettingID portTooltipsSettingID = "port_tooltips";

private:
    void setScaleFactor (float newScaleFactor);
    void toggleTooltips (bool shouldShow);

    BoardComponent comp;

    DrawableButton plusButton { "", DrawableButton::ImageOnButtonBackground };
    DrawableButton minusButton { "", DrawableButton::ImageOnButtonBackground };

    Label scaleLabel;
    Value scaleFactor { 1.0f };

    std::optional<TooltipWindow> tooltips;

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardViewport)
};
