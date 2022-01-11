#pragma once

#include "../BYOD.h"
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

    TextButton plusButton { "+" };
    TextButton minusButton { "-" };

    Label scaleLabel;

    float scaleFactor = 1.0f;

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardViewport)
};

class BoardItem : public foleys::GuiItem
{
public:
    FOLEYS_DECLARE_GUI_FACTORY (BoardItem)

    BoardItem (foleys::MagicGUIBuilder& builder, const ValueTree& node) : foleys::GuiItem (builder, node)
    {
        auto* plugin = dynamic_cast<BYOD*> (builder.getMagicState().getProcessor());
        view = std::make_unique<BoardViewport> (plugin->getProcChain());
        addAndMakeVisible (view.get());
    }

    void update() override
    {
        view->repaint();
    }

    Component* getWrappedComponent() override
    {
        return view.get();
    }

private:
    std::unique_ptr<BoardViewport> view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardItem)
};
