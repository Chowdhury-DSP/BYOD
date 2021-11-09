#pragma once

#include "../BYOD.h"
#include "BoardComponent.h"

class BoardViewport : public Viewport
{
public:
    BoardViewport (ProcessorChain& procChain);
    ~BoardViewport();

    void resized() override;

private:
    BoardComponent comp;

    TextButton plusButton { "+" };
    TextButton minusButton { "-" };

    Label scaleLabel;

    float scaleFactor = 1.0f;

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
