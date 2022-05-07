#include "LookAndFeels.h"

//==================================================================
void ByodLNF::drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, const bool isSeparator, const bool isActive, const bool isHighlighted, const bool /*isTicked*/, const bool hasSubMenu, const String& text, const String& shortcutKeyText, const Drawable* icon, const Colour* const textColourToUse)
{
    LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive, isHighlighted, false /*isTicked*/, hasSubMenu, text, shortcutKeyText, icon, textColourToUse);
}

void ByodLNF::drawPopupMenuBackground (Graphics& g, int width, int height)
{
    g.fillAll (findColour (PopupMenu::backgroundColourId));
    ignoreUnused (width, height);
}

void ByodLNF::drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
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

Font ByodLNF::getTextButtonFont (TextButton&, int buttonHeight)
{
    return Font (jmin (17.0f, (float) buttonHeight * 0.8f)).boldened();
}

//==================================================================
ProcessorLNF::ProcessorLNF()
{
    setColour (ComboBox::backgroundColourId, Colours::transparentBlack);
    setColour (ComboBox::outlineColourId, Colours::darkgrey);
}

void ProcessorLNF::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (1, 1, box.getWidth() - 2, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box).boldened());
    label.setJustificationType (Justification::centred);
}

void ProcessorLNF::drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
    auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
    Rectangle<int> boxBounds (0, 0, width, height);

    g.setColour (box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);

    g.setColour (box.findColour (ComboBox::outlineColourId));
    g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);
}

Font ProcessorLNF::getComboBoxFont (ComboBox& box)
{
    const auto textWidth = box.getWidth() - 4;
    auto font = Font ((float) box.getHeight() * 0.5f);

    while (font.getStringWidth (box.getText()) > textWidth)
        font = Font (font.getHeight() - 0.5f);

    return font;
}

Slider::SliderLayout ProcessorLNF::getSliderLayout (Slider& slider)
{
    auto layout = ByodLNF::getSliderLayout (slider);

    auto labelWidth = slider.isVertical() ? slider.getWidth() : slider.proportionOfWidth (labelWidthPct);
    auto centre = layout.textBoxBounds.getCentre();
    layout.textBoxBounds = layout.textBoxBounds.withWidth (labelWidth)
                               .withCentre (centre);

    return layout;
}

class LabelWithCustomEditor : public Label
{
public:
    LabelWithCustomEditor() : Label ({}, {}) {}

    std::function<void (TextEditor&)> customiseEditor = [] (TextEditor&) {};

    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails&) override {}

    TextEditor* createEditorComponent() override
    {
        auto* editor = Label::createEditorComponent();

        if (editor != nullptr)
            customiseEditor (*editor);

        return editor;
    }
};

Label* ProcessorLNF::createSliderTextBox (Slider& slider)
{
    auto l = new LabelWithCustomEditor();

    l->setFont ((float) slider.getTextBoxHeight());
    l->setJustificationType (Justification::centred);
    l->setKeyboardType (TextInputTarget::decimalKeyboard);

    l->setColour (Label::textColourId, slider.findColour (Slider::textBoxTextColourId));
    l->setColour (Label::textWhenEditingColourId, slider.findColour (Slider::textBoxTextColourId));
    l->setColour (Label::backgroundColourId, slider.findColour (Slider::textBoxBackgroundColourId));
    l->setColour (Label::outlineColourId, slider.findColour (Slider::textBoxOutlineColourId));
    l->setColour (TextEditor::textColourId, slider.findColour (Slider::textBoxTextColourId));
    l->setColour (TextEditor::backgroundColourId, slider.findColour (Slider::textBoxBackgroundColourId).withAlpha (1.0f));
    l->setColour (TextEditor::outlineColourId, slider.findColour (Slider::textBoxOutlineColourId));
    l->setColour (TextEditor::highlightColourId, slider.findColour (Slider::textBoxHighlightColourId));

    l->customiseEditor = [&] (TextEditor& ed)
    {
        ed.setFont (0.85f * (float) slider.getTextBoxHeight());
        ed.setJustification (Justification::centred);

        ed.setColour (TextEditor::backgroundColourId, Colours::transparentWhite);
        ed.setColour (TextEditor::focusedOutlineColourId, Colours::transparentWhite);
        ed.setColour (TextEditor::highlightedTextColourId, slider.findColour (Slider::textBoxHighlightColourId).contrasting());
        ed.setColour (CaretComponent::caretColourId, slider.findColour (Slider::textBoxHighlightColourId).withAlpha (0.9f));
    };

    return l;
}

Font ProcessorLNF::getLabelFont (Label& label)
{
    auto* labelParent = dynamic_cast<Slider*> (label.getParentComponent());
    if (labelParent == nullptr)
        return ByodLNF::getLabelFont (label);

    const auto textWidth = label.getWidth() - 2;
    auto font = Font ((float) label.getHeight() * 0.8f);

    while (font.getStringWidth (label.getText()) > textWidth)
        font = Font (font.getHeight() - 0.5f);

    return font;
}

void ProcessorLNF::drawLabel (Graphics& g, Label& label)
{
    g.fillAll (label.findColour (Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const Font font (getLabelFont (label));

        g.setColour (label.findColour (Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (font);

        auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());

        g.drawFittedText (label.getText(), textArea, label.getJustificationType(), jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())), label.getMinimumHorizontalScale());

        g.setColour (label.findColour (Label::outlineColourId).withMultipliedAlpha (alpha));
    }
    else if (label.isEnabled())
    {
        g.setColour (label.findColour (Label::outlineColourId));
    }

    g.drawRoundedRectangle (label.getLocalBounds().toFloat().reduced (0.5f), 3.0f, 1.0f);
}

//==================================================================
int BottomBarLNF::getNameWidth (int height, const String& text)
{
    Font f = getFontForSliderHeight (height);
    return f.getStringWidth (text);
}

Font BottomBarLNF::getFontForSliderHeight (int height)
{
    return { jmin ((float) height * 0.5f, 19.0f) };
}

void BottomBarLNF::drawRotarySlider (Graphics& g, int, int, int, int height, float, const float, const float, Slider& slider)
{
    g.setColour (Colours::white); // @TODO: make colour selectable
    g.setFont (getFontForSliderHeight (height).boldened());

    String text = slider.getName() + ": ";
    int width = getNameWidth (height, text);
    g.drawFittedText (text, 0, 0, width, height, Justification::centred, 1);
}

Slider::SliderLayout BottomBarLNF::getSliderLayout (Slider& slider)
{
    auto layout = LookAndFeel_V4::getSliderLayout (slider);
    layout.textBoxBounds = slider.getLocalBounds()
                               .removeFromRight (slider.getWidth()
                                                 - getNameWidth (slider.getHeight(), slider.getName() + ":_") + 3);
    return layout;
}

Label* BottomBarLNF::createSliderTextBox (Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setInterceptsMouseClicks (false, false);
    label->setColour (Label::outlineColourId, Colours::transparentBlack);
    label->setColour (Label::outlineWhenEditingColourId, Colours::transparentBlack);
    label->setJustificationType (Justification::left);
    label->setFont (getFontForSliderHeight (slider.getHeight()).boldened());

    label->onEditorShow = [label]
    {
        if (auto* editor = label->getCurrentTextEditor())
        {
            editor->setBounds (label->getLocalBounds());
            editor->setColour (CaretComponent::caretColourId, Colour (0xFFC954D4));
            editor->setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
            editor->setJustification (Justification::left);
            editor->applyFontToAllText (getFontForSliderHeight (editor->getHeight()).boldened());
        }
    };

    return label;
}
