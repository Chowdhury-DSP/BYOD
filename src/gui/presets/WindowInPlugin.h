#pragma once

#include <pch.h>

template <typename OwnedCompType>
class WindowInPlugin : public Component,
                       private ComponentListener
{
public:
    static_assert (std::is_base_of<Component, OwnedCompType>::value, "Owned Component buse be derived from juce::Component");

    // @TODO enable_if default constructible
    WindowInPlugin (Component& creatorComponent) : creatorComp (creatorComponent)
    {
        initialise();
    }

    template <typename... Args>
    WindowInPlugin (Component& creatorComponent, Args... args) : creatorComp (creatorComponent),
                                                                 viewComponent (std::forward (args...))
    {
        initialise();
    }

    ~WindowInPlugin() override
    {
        creatorComp.removeComponentListener (this);
    }

    OwnedCompType& getViewComponent() { return viewComponent; }

    void show()
    {
        setBoundsBeforeShowing();
        toFront (true);
        setVisible (true);
    }

    void setBoundsBeforeShowing()
    {
        auto* parent = getParentComponent();
        jassert (parent != nullptr); // trying to show before adding a parent??

        auto parentBounds = parent->getLocalBounds();

        auto bounds = Rectangle { viewComponent.getWidth(), viewComponent.getHeight() + nameHeight }.expanded (1);
        setBounds (bounds.withCentre (parentBounds.getCentre()));
    }

    void componentParentHierarchyChanged (Component& comp) override
    {
        if (&creatorComp != &comp)
            return;

        auto* topLevelComp = creatorComp.getTopLevelComponent();
        jassert (topLevelComp != nullptr);

        if (auto* parent = getParentComponent())
            parent->removeChildComponent (this);

        topLevelComp->addChildComponent (this);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::darkgrey);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto topBounds = bounds.removeFromTop (nameHeight);
        closeButton.setBounds (topBounds.removeFromRight (nameHeight).reduced (1));

        bounds.reduce (1, 1);
        viewComponent.setTopLeftPosition (bounds.getTopLeft());
    }

private:
    void initialise()
    {
        creatorComp.addChildComponent (this);
        creatorComp.addComponentListener (this);

        closeButton.setColour (TextButton::buttonColourId, Colours::red);
        closeButton.setColour (TextButton::textColourOffId, Colours::black);
        closeButton.onClick = [&]
        { setVisible (false); };
        addAndMakeVisible (closeButton);

        addAndMakeVisible (viewComponent);
    }

    static constexpr int nameHeight = 25;

    Component& creatorComp;
    OwnedCompType viewComponent;

    TextButton closeButton { "X" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowInPlugin)
};
