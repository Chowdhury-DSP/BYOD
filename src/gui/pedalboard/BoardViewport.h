#pragma once

#include "BYOD.h"
#include "BoardComponent.h"

class BoardViewport : public Viewport,
                      private chowdsp::GlobalPluginSettings::Listener
{
public:
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;

    explicit BoardViewport (ProcessorChain& procChain);
    ~BoardViewport() override;

    void resized() override;

    void globalSettingChanged (SettingID settingID) override;

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
