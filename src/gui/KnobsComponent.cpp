#include "KnobsComponent.h"

KnobsComponent::KnobsComponent (AudioProcessorValueTreeState& vts, const Colour& c, std::function<void()> paramLambda) : colour (c)
{
    auto addSlider = [=, &vts] (AudioParameterFloat* param) {
        auto newSlide = std::make_unique<SliderWithAttachment>();
        addAndMakeVisible (newSlide->slider);
        newSlide->attachment = std::make_unique<SliderAttachment> (vts, param->paramID, newSlide->slider);

        newSlide->slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        newSlide->slider.setName (param->name);
        newSlide->slider.setTextBoxStyle (Slider::TextBoxBelow, false, 75, 16);
        newSlide->slider.setColour (Slider::textBoxOutlineColourId, colour);
        newSlide->slider.setColour (Slider::textBoxTextColourId, colour);
        newSlide->slider.onValueChange = paramLambda;

        sliders.add (std::move (newSlide));
    };

    auto addBox = [=, &vts] (AudioParameterChoice* param) {
        auto newBox = std::make_unique<BoxWithAttachment>();
        addAndMakeVisible (newBox->box);
        newBox->box.setName (param->name);
        newBox->box.addItemList (param->choices, 1);
        newBox->box.setSelectedItemIndex (0);
        newBox->box.setColour (ComboBox::outlineColourId, colour);
        newBox->box.setColour (ComboBox::textColourId, colour);
        newBox->box.setColour (ComboBox::arrowColourId, colour);
        newBox->box.onChange = paramLambda;

        newBox->attachment = std::make_unique<ComboBoxAttachment> (vts, param->paramID, newBox->box);

        boxes.add (std::move (newBox));
    };

    auto addButton = [=, &vts] (AudioParameterBool* param) {
        if (param->paramID == "on_off")
            return;

        auto newButton = std::make_unique<ButtonWithAttachment>();
        addAndMakeVisible (newButton->button);
        newButton->button.setButtonText (param->name);
        newButton->button.setClickingTogglesState (true);
        newButton->button.setColour (TextButton::buttonOnColourId, Colours::red);
        newButton->button.onStateChange = paramLambda;

        newButton->attachment = std::make_unique<ButtonAttachment> (vts, param->paramID, newButton->button);

        buttons.add (std::move (newButton));
    };

    for (auto* param : vts.processor.getParameters())
    {
        if (auto* paramFloat = dynamic_cast<AudioParameterFloat*> (param))
            addSlider (paramFloat);

        else if (auto* paramChoice = dynamic_cast<AudioParameterChoice*> (param))
            addBox (paramChoice);

        else if (auto* paramBool = dynamic_cast<AudioParameterBool*> (param))
            addButton (paramBool);
    }

    setSize (getWidth(), 100);
}

void KnobsComponent::paint (Graphics& g)
{
    g.setColour (colour.withAlpha (isEnabled() ? 1.0f : 0.6f));
    auto makeName = [&g] (Component& comp, String name, int offset = 0) {
        const int height = 20;
        g.setFont (Font (18.0f).boldened());
        Rectangle<int> nameBox (comp.getX(), comp.getY() - 22 + offset, comp.getWidth(), height);
        g.drawFittedText (name, nameBox, Justification::centred, 1);
    };

    for (auto* s : sliders)
        makeName (s->slider, s->slider.getName(), 6);

    for (auto* b : boxes)
        makeName (b->box, b->box.getName());
}

void KnobsComponent::resized()
{
    int totalNumComponents = sliders.size() + boxes.size() + buttons.size();

    int compHeight = getHeight() - 20;
    int compWidth = totalNumComponents > 1 ? (getWidth() - 10) / totalNumComponents : compHeight;

    if (totalNumComponents == 1)
    {
        const int x = (getWidth() - compWidth) / 2;
        for (auto* s : sliders)
            s->slider.setBounds (x, 15, compWidth, compWidth);

        for (auto* b : boxes)
            b->box.setBounds (x, 15 + (compHeight - 30) / 2, compWidth - 5, 30);

        for (auto* b : buttons)
            b->button.setBounds (x, 15 + (compHeight - 30) / 2, compWidth - 5, 30);

        return;
    }

    if (totalNumComponents == 2)
    {
        int compIdx = 0;
        const int x = (getWidth() - 2 * compWidth) / 2;

        for (auto* s : sliders)
            s->slider.setBounds (x + (compIdx++) * compWidth, 15, compWidth, compWidth);

        for (auto* b : boxes)
            b->box.setBounds (x + (compIdx++) * compWidth, 15 + (compHeight - 30) / 2, compWidth - 5, 30);

        for (auto* b : buttons)
            b->button.setBounds (x + (compIdx++) * compWidth, 15 + (compHeight - 30) / 2, compWidth - 5, 30);

        return;
    }

    if (totalNumComponents == 3)
    {
        int compIdx = 0;
        const int x = (getWidth() - 3 * compWidth) / 2;
        const int y = 30;

        for (auto* s : sliders)
            s->slider.setBounds (x + (compIdx++) * compWidth, y, compWidth, compWidth);

        for (auto* b : boxes)
            b->box.setBounds (x + (compIdx++) * compWidth, y + (compHeight - 60) / 2, compWidth - 5, 30);

        for (auto* b : buttons)
            b->button.setBounds (x + (compIdx++) * compWidth, y + (compHeight - 60) / 2, compWidth - 5, 30);

        return;
    }

    if (totalNumComponents == 3)
    {
        int compIdx = 0;
        const int x = (getWidth() - 3 * compWidth) / 2;
        const int y = 30;

        for (auto* s : sliders)
            s->slider.setBounds (x + (compIdx++) * compWidth, y, compWidth, compWidth);

        for (auto* b : boxes)
            b->box.setBounds (x + (compIdx++) * compWidth, y + (compHeight - 60) / 2, compWidth - 5, 30);

        for (auto* b : buttons)
            b->button.setBounds (x + (compIdx++) * compWidth, y + (compHeight - 60) / 2, compWidth - 5, 30);

        return;
    }

    if (totalNumComponents == 4)
    {
        int compIdx = 0;

        Rectangle<int> bounds[4];
        bounds[0] = Rectangle<int> { 0, 15, 100, 100 };
        bounds[1] = Rectangle<int> { 72, 80, 100, 100 };
        bounds[2] = Rectangle<int> { 144, 15, 100, 100 };
        bounds[3] = Rectangle<int> { 216, 80, 100, 100 };

        for (auto* s : sliders)
            s->slider.setBounds (bounds[compIdx++]);

        for (auto* b : boxes)
            b->box.setBounds (bounds[compIdx++].withHeight (30).translated (0, 50));

        for (auto* b : buttons)
            b->button.setBounds (bounds[compIdx++].withHeight (30).translated (0, 50));

        return;
    }
}
