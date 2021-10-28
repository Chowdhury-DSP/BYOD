#include "KnobsComponent.h"
#include "processors/BaseProcessor.h"

namespace
{
constexpr int nameHeight = 16;
}

KnobsComponent::KnobsComponent (BaseProcessor& baseProc, AudioProcessorValueTreeState& vts, const Colour& cc, const Colour& ac, std::function<void()> paramLambda)
    : contrastColour (cc), accentColour (ac)
{
    auto addSlider = [=, &vts] (AudioParameterFloat* param) {
        auto newSlide = std::make_unique<SliderWithAttachment>();
        addAndMakeVisible (newSlide.get());
        newSlide->attachment = std::make_unique<SliderAttachment> (vts, param->paramID, *newSlide.get());

        newSlide->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        newSlide->setName (param->name);
        newSlide->setTextBoxStyle (Slider::TextBoxBelow, false, 66, 16);
        newSlide->setColour (Slider::textBoxOutlineColourId, contrastColour);
        newSlide->setColour (Slider::textBoxTextColourId, contrastColour);
        newSlide->setColour (Slider::thumbColourId, accentColour);
        newSlide->onValueChange = paramLambda;

        sliders.add (std::move (newSlide));
    };

    auto addBox = [=, &vts] (AudioParameterChoice* param) {
        auto newBox = std::make_unique<BoxWithAttachment>();
        addAndMakeVisible (newBox.get());
        newBox->setName (param->name);
        newBox->addItemList (param->choices, 1);
        newBox->setSelectedItemIndex (0);
        newBox->setColour (ComboBox::outlineColourId, contrastColour);
        newBox->setColour (ComboBox::textColourId, contrastColour);
        newBox->setColour (ComboBox::arrowColourId, contrastColour);
        newBox->onChange = paramLambda;

        newBox->attachment = std::make_unique<ComboBoxAttachment> (vts, param->paramID, *newBox.get());

        boxes.add (std::move (newBox));
    };

    auto addButton = [=, &vts] (AudioParameterBool* param) {
        if (param->paramID == "on_off")
            return;

        auto newButton = std::make_unique<ButtonWithAttachment>();
        addAndMakeVisible (newButton.get());
        newButton->setButtonText (param->name);
        newButton->setClickingTogglesState (true);
        newButton->setColour (TextButton::buttonOnColourId, Colours::red);
        newButton->onStateChange = paramLambda;

        newButton->attachment = std::make_unique<ButtonAttachment> (vts, param->paramID, *newButton.get());

        buttons.add (std::move (newButton));
    };

    baseProc.getCustomComponents (customComponents);

    for (auto* param : vts.processor.getParameters())
    {
        if (auto* rangedParam = dynamic_cast<RangedAudioParameter*> (param))
        {
            bool found = false;
            for (auto* comp : customComponents)
            {
                if (comp->getName().contains (rangedParam->paramID + "__"))
                {
                    comp->setName (rangedParam->name);
                    found = true;
                    break;
                }
            }

            if (found)
                continue;
        }

        if (auto* paramFloat = dynamic_cast<AudioParameterFloat*> (param))
            addSlider (paramFloat);

        else if (auto* paramChoice = dynamic_cast<AudioParameterChoice*> (param))
            addBox (paramChoice);

        else if (auto* paramBool = dynamic_cast<AudioParameterBool*> (param))
            addButton (paramBool);
    }

    for (auto* comp : customComponents)
    {
        addAndMakeVisible (comp);
        if (auto* boxComp = dynamic_cast<ComboBox*> (comp))
        {
            boxComp->setColour (ComboBox::outlineColourId, contrastColour);
            boxComp->setColour (ComboBox::textColourId, contrastColour);
            boxComp->setColour (ComboBox::arrowColourId, contrastColour);

            customComponents.removeObject (comp, false);
            boxes.add (boxComp);
        }
    }

    setSize (getWidth(), 100);
}

void KnobsComponent::paint (Graphics& g)
{
    g.setColour (contrastColour.withAlpha (isEnabled() ? 1.0f : 0.6f));
    auto makeName = [&g] (Component& comp, String name, int offset = 0) {
        g.setFont (Font ((float) nameHeight - 2.0f).boldened());
        Rectangle<int> nameBox (comp.getX(), comp.getY() - 22 + offset, comp.getWidth(), nameHeight);
        g.drawFittedText (name, nameBox, Justification::centred, 1);
    };

    for (auto* s : sliders)
        makeName (*s, s->getName(), 6);

    for (auto* b : boxes)
        makeName (*b, b->getName());
}

void KnobsComponent::resized()
{
    int totalNumComponents = sliders.size() + boxes.size() + buttons.size();

    int compHeight = getHeight() - nameHeight;
    int compWidth = totalNumComponents > 1 ? (getWidth() - 10) / totalNumComponents : compHeight;

    if (totalNumComponents == 1)
    {
        const int x = (getWidth() - compWidth) / 2;
        if (sliders.size() > 0)
            sliders[0]->setBounds (x, 15, compWidth - 5, compWidth - 5);

        if (boxes.size() > 0)
        {
            compWidth = getWidth() - 30;
            boxes[0]->setBounds (Rectangle (compWidth, 30).withCentre (getLocalBounds().getCentre()));
        }

        if (buttons.size() > 0)
            buttons[0]->setBounds (x, 15 + (compHeight - 30) / 2, compWidth - 5, 30);

        if (customComponents.size() > 0)
            customComponents[0]->setBounds (x, 15 + (compHeight - 30) / 2, compWidth / -5, 30);

        return;
    }

    if (totalNumComponents == 2)
    {
        int compIdx = 0;
        const int x = (getWidth() - 2 * compWidth) / 2;

        for (auto* s : sliders)
            s->setBounds (x + (compIdx++) * compWidth, 15, compWidth, compWidth);

        for (auto* b : boxes)
            b->setBounds (x + (compIdx++) * compWidth, 15 + (compHeight - 30) / 2, compWidth - 5, 30);

        for (auto* b : buttons)
            b->setBounds (x + (compIdx++) * compWidth, 15 + (compHeight - 30) / 2, compWidth - 5, 30);

        return;
    }

    if (totalNumComponents == 3)
    {
        int compIdx = 0;
        const int x = (getWidth() - 3 * compWidth) / 2;
        const int y = 30;

        for (auto* s : sliders)
            s->setBounds (x + (compIdx++) * compWidth, y, compWidth, compWidth);

        for (auto* b : boxes)
            b->setBounds (x + (compIdx++) * compWidth, y + (compHeight - 60) / 2, compWidth - 5, 30);

        for (auto* b : buttons)
            b->setBounds (x + (compIdx++) * compWidth, y + (compHeight - 60) / 2, compWidth - 5, 30);

        return;
    }

    if (totalNumComponents == 4)
    {
        int compIdx = 0;

        Rectangle<int> bounds[4];
        constexpr int sWidth = 80;
        bounds[0] = Rectangle<int> { 0, 20, sWidth, sWidth };
        bounds[1] = Rectangle<int> { 60, 40, sWidth, sWidth };
        bounds[2] = Rectangle<int> { 120, 20, sWidth, sWidth };
        bounds[3] = Rectangle<int> { 180, 40, sWidth, sWidth };

        for (auto* s : sliders)
        {
            s->setBounds (bounds[compIdx++]);
            s->setTextBoxStyle (Slider::TextBoxBelow, false, sWidth - 15, 16);
        }

        for (auto* b : boxes)
            b->setBounds (bounds[compIdx++].withHeight (30).translated (-5, 62));

        for (auto* b : buttons)
            b->setBounds (bounds[compIdx++].withHeight (30).translated (0, 50));

        return;
    }

    if (totalNumComponents == 5)
    {
        int compIdx = 0;

        Rectangle<int> bounds[5];
        constexpr int sWidth = 75;
        bounds[0] = Rectangle<int> { 0, 65, sWidth, sWidth };
        bounds[1] = Rectangle<int> { 58, 15, sWidth, sWidth };
        bounds[2] = Rectangle<int> { 115, 65, sWidth, sWidth };
        bounds[3] = Rectangle<int> { 180, 65, sWidth, sWidth };
        bounds[4] = Rectangle<int> { 145, 25, 90, 30 };

        for (auto* s : sliders)
        {
            s->setBounds (bounds[compIdx++]);
            s->setTextBoxStyle (Slider::TextBoxBelow, false, sWidth - 10, 16);
        }

        for (auto* b : boxes)
            b->setBounds (bounds[compIdx++].withHeight (25));

        for (auto* b : buttons)
            b->setBounds (bounds[compIdx++].withHeight (30).translated (0, 50));

        return;
    }

    if (totalNumComponents > 5)
    {
        auto width = (getWidth() - 10) / totalNumComponents;
        int x = 5;
        for (auto* s : sliders)
        {
            s->setSliderStyle (Slider::SliderStyle::LinearVertical);
            s->setTextBoxStyle (Slider::TextBoxBelow, false, width - 2, 14);
            auto bounds = Rectangle { x + 2, 15, width - 4, getHeight() - 15 };
            x += width;

            s->setBounds (bounds);
        }
    }
}
