#pragma once

#include <pch.h>

class ByodLNF : public chowdsp::ChowLNF
{
public:
    ByodLNF() = default;
    ~ByodLNF() override = default;

    void drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, const bool isSeparator, const bool isActive, const bool isHighlighted, const bool /*isTicked*/, const bool hasSubMenu, const String& text, const String& shortcutKeyText, const Drawable* icon, const Colour* const textColourToUse) override
    {
        LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive, isHighlighted, false /*isTicked*/, hasSubMenu, text, shortcutKeyText, icon, textColourToUse);
    }

    Component* getParentComponentForMenuOptions (const PopupMenu::Options& options) override
    {
#if JUCE_IOS
        if (PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_AudioUnitv3)
        {
            if (options.getParentComponent() == nullptr && options.getTargetComponent() != nullptr)
                return options.getTargetComponent()->getTopLevelComponent();
        }
#endif
        return LookAndFeel_V2::getParentComponentForMenuOptions (options);
    }

    void drawPopupMenuBackground (Graphics& g, int width, int height) override
    {
        g.fillAll (findColour (PopupMenu::backgroundColourId));
        ignoreUnused (width, height);
    }

    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        constexpr auto cornerSize = 3.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

        auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                              .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting (shouldDrawButtonAsDown ? 0.2f : 0.05f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (bounds, cornerSize);
    }

    Font getTextButtonFont (TextButton&, int buttonHeight) override
    {
        return Font (jmin (17.0f, (float) buttonHeight * 0.8f)).boldened();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ByodLNF)
};

class ProcessorLNF : public ByodLNF // public foleys::LookAndFeel
{
public:
    ProcessorLNF()
    {
        setColour (ComboBox::backgroundColourId, Colours::transparentBlack);
        setColour (ComboBox::outlineColourId, Colours::darkgrey);
    }

    void positionComboBoxText (ComboBox& box, Label& label) override
    {
        label.setBounds (1, 1, box.getWidth() - 30, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box).boldened());
        label.setJustificationType (Justification::centred);
    }

    void drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box) override
    {
        auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
        Rectangle<int> boxBounds (0, 0, width, height);

        g.setColour (box.findColour (ComboBox::backgroundColourId));
        g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);

        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

        Rectangle<int> arrowZone (width - 30, 0, 20, height);
        Path path;
        path.startNewSubPath ((float) arrowZone.getX() + 3.0f, (float) arrowZone.getCentreY() - 2.0f);
        path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
        path.lineTo ((float) arrowZone.getRight() - 3.0f, (float) arrowZone.getCentreY() - 2.0f);

        g.setColour (box.findColour (ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath (path, PathStrokeType (2.0f));
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorLNF)
};

//==================================================================
/** Look and feel for the bottom bar section */
class BottomBarLNF : public chowdsp::ChowLNF
{
public:
    BottomBarLNF() = default;
    virtual ~BottomBarLNF() = default;

protected:
    static constexpr float heightFrac = 0.5f;

    int getNameWidth (int height, const String& text)
    {
        Font f = Font ((float) height * heightFrac);
        return f.getStringWidth (text);
    }

    void drawRotarySlider (Graphics& g, int, int, int, int height, float, const float, const float, Slider& slider) override
    {
        g.setColour (Colours::white); // @TODO: make colour selectable
        g.setFont (Font ((float) height * heightFrac).boldened());

        String text = slider.getName() + ": ";
        int width = getNameWidth (height, text);
        g.drawFittedText (text, 0, 0, width, height, Justification::centred, 1);
    }

    Slider::SliderLayout getSliderLayout (Slider& slider) override
    {
        auto layout = LookAndFeel_V4::getSliderLayout (slider);
        layout.textBoxBounds = slider.getLocalBounds()
                                   .removeFromRight (slider.getWidth()
                                                     - getNameWidth (slider.getHeight(), slider.getName() + ":_") + 3);
        return layout;
    }

    Label* createSliderTextBox (Slider& slider) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox (slider);
        label->setInterceptsMouseClicks (false, false);
        label->setColour (Label::outlineColourId, Colours::transparentBlack);
        label->setColour (Label::outlineWhenEditingColourId, Colours::transparentBlack);
        label->setJustificationType (Justification::centred);
        label->setFont (Font (16.0f).boldened());

        label->onEditorShow = [label]
        {
            if (auto editor = label->getCurrentTextEditor())
            {
                editor->setBounds (label->getLocalBounds());
                editor->setColour (CaretComponent::caretColourId, Colour (0xFFC954D4));
                editor->setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
                editor->setJustification (Justification::left);
                editor->applyFontToAllText (Font (14.0f).boldened());
            }
        };

        return label;
    }
};

//==================================================================
class LNFAllocator
{
public:
    LNFAllocator()
    {
        lnfs.ensureStorageAllocated (24);
    }

    ~LNFAllocator()
    {
        LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    LookAndFeel* addLookAndFeel (LookAndFeel* laf)
    {
        jassert (laf != nullptr);
        jassert (! lnfs.contains (laf));

        return lnfs.add (laf);
    }

    template <typename LookAndFeelSubclass>
    bool containsLookAndFeelType() const
    {
        for (auto* l : lnfs)
            if (dynamic_cast<LookAndFeelSubclass*> (l) != nullptr)
                return true;

        return false;
    }

    template <typename LookAndFeelSubclass>
    LookAndFeelSubclass* getLookAndFeel()
    {
        for (auto* l : lnfs)
            if (auto* result = dynamic_cast<LookAndFeelSubclass*> (l))
                return result;

        return static_cast<LookAndFeelSubclass*> (lnfs.add (std::make_unique<LookAndFeelSubclass>()));
    }

private:
    OwnedArray<LookAndFeel> lnfs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LNFAllocator)
};
