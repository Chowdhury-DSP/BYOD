#pragma once

#include "BYOD.h"
#include "BoardComponent.h"

class BoardViewport : public Viewport,
                      public chowdsp::TrackedByBroadcasters
{
public:
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;

    BoardViewport (ProcessorChain& procChain, chowdsp::HostContextProvider& hostContextProvider);

    void resized() override;

    void globalSettingChanged (SettingID settingID);

    static constexpr SettingID defaultZoomSettingID = "default_zoom";

private:
    void setScaleFactor (float newScaleFactor);

    BoardComponent comp;

    DrawableButton plusButton { "", DrawableButton::ImageOnButtonBackground };
    DrawableButton minusButton { "", DrawableButton::ImageOnButtonBackground };

    Label scaleLabel;

    float scaleFactor = 1.0f;

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardViewport)
};
