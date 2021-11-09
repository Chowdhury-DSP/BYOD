#include "KnobsComponent.h"
#include "processors/BaseProcessor.h"

namespace
{
constexpr float nameHeightScale = 0.115f;
}

KnobsComponent::KnobsComponent (BaseProcessor& baseProc, AudioProcessorValueTreeState& vts, const Colour& cc, const Colour& ac, std::function<void()> paramLambda)
    : contrastColour (cc), accentColour (ac)
{
    auto addSlider = [=, &vts] (AudioParameterFloat* param)
    {
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

    auto addBox = [=, &vts] (AudioParameterChoice* param)
    {
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

    auto addButton = [=, &vts] (AudioParameterBool* param)
    {
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

    const auto nameHeight = proportionOfHeight (nameHeightScale);
    const auto nameOffset = proportionOfHeight (0.157f);
    auto makeName = [&g, nameHeight, nameOffset] (Component& comp, String name, int offset = 0)
    {
        g.setFont (Font ((float) nameHeight - 2.0f).boldened());
        Rectangle<int> nameBox (comp.getX(), comp.getY() - nameOffset + offset, comp.getWidth(), nameHeight);
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

    const auto nameHeight = proportionOfHeight (nameHeightScale);
    int compHeight = getHeight() - nameHeight;
    int compWidth = totalNumComponents > 1 ? (getWidth() - 10) / totalNumComponents : compHeight;

    if (totalNumComponents == 1)
    {
        const int x = (getWidth() - compWidth) / 2;
        const auto yPad = proportionOfHeight (0.107f);
        const auto yDim = proportionOfHeight (0.214f);
        if (sliders.size() > 0)
            sliders[0]->setBounds (x, yPad, compWidth - 5, compWidth - 5);

        if (boxes.size() > 0)
        {
            compWidth = getWidth() - yDim;
            boxes[0]->setBounds (Rectangle (compWidth, yDim).withCentre (getLocalBounds().getCentre()));
        }

        if (buttons.size() > 0)
            buttons[0]->setBounds (x, yPad + (compHeight - yDim) / 2, compWidth - 5, yDim);

        if (customComponents.size() > 0)
            customComponents[0]->setBounds (x, yPad + (compHeight - yDim) / 2, compWidth / -5, yDim);

        return;
    }

    if (totalNumComponents == 2)
    {
        int compIdx = 0;
        const int x = (getWidth() - 2 * compWidth) / 2;
        const auto yPad = proportionOfHeight (0.107f);
        const auto yDim = proportionOfHeight (0.214f);

        for (auto* s : sliders)
            s->setBounds (x + (compIdx++) * compWidth, yPad, compWidth, compWidth);

        for (auto* b : boxes)
            b->setBounds (x + (compIdx++) * compWidth, yPad + (compHeight - yDim) / 2, compWidth - 5, yDim);

        for (auto* b : buttons)
            b->setBounds (x + (compIdx++) * compWidth, yPad + (compHeight - yDim) / 2, compWidth - 5, yDim);

        return;
    }

    if (totalNumComponents == 3)
    {
        int compIdx = 0;
        const int x = (getWidth() - 3 * compWidth) / 2;
        const auto yDim = proportionOfHeight (0.214f);

        for (auto* s : sliders)
            s->setBounds (x + (compIdx++) * compWidth, yDim, compWidth, compWidth);

        for (auto* b : boxes)
            b->setBounds (x + (compIdx++) * compWidth, yDim + (compHeight - 2 * yDim) / 2, compWidth - 5, yDim);

        for (auto* b : buttons)
            b->setBounds (x + (compIdx++) * compWidth, yDim + (compHeight - 2 * yDim) / 2, compWidth - 5, yDim);

        return;
    }

    if (totalNumComponents == 4)
    {
        int compIdx = 0;

        Rectangle<int> bounds[4];
        const int sWidth = proportionOfWidth (0.308f);
        const int xOff = proportionOfWidth (0.231f);
        const int yOff = proportionOfHeight (0.143f);
        bounds[0] = Rectangle<int> { 0, yOff, sWidth, sWidth };
        bounds[1] = Rectangle<int> { xOff, 2 * yOff, sWidth, sWidth };
        bounds[2] = Rectangle<int> { 2 * xOff, yOff, sWidth, sWidth };
        bounds[3] = Rectangle<int> { 3 * xOff, 2 * yOff, sWidth, sWidth };

        for (auto* s : sliders)
        {
            s->setBounds (bounds[compIdx++]);
            s->setTextBoxStyle (Slider::TextBoxBelow, false, sWidth - 15, 16);
        }

        const auto yDim = proportionOfHeight (0.214f);
        for (auto* b : boxes)
            b->setBounds (bounds[compIdx++].withHeight (yDim).translated (-5, 2 * yDim));

        for (auto* b : buttons)
            b->setBounds (bounds[compIdx++].withHeight (yDim).translated (0, yDim * 10 / 6));

        return;
    }

    if (totalNumComponents == 5)
    {
        int compIdx = 0;

        Rectangle<int> bounds[5];
        const int sWidth = proportionOfWidth (0.288f);
        const int y1 = proportionOfHeight (0.464f);
        const int y2 = proportionOfHeight (0.107f);
        const int y3 = proportionOfHeight (0.179f);
        const int x1 = proportionOfWidth (0.223f);
        const int x2 = proportionOfWidth (0.442f);
        const int x3 = proportionOfWidth (0.692f);
        const int x4 = proportionOfWidth (0.558f);

        const int xDim = proportionOfWidth (0.346f);
        const int yDim = proportionOfHeight (0.214f);

        bounds[0] = Rectangle<int> { 0, y1, sWidth, sWidth };
        bounds[1] = Rectangle<int> { x1, y2, sWidth, sWidth };
        bounds[2] = Rectangle<int> { x2, y1, sWidth, sWidth };
        bounds[3] = Rectangle<int> { x3, y1, sWidth, sWidth };
        bounds[4] = Rectangle<int> { x4, y3, xDim, yDim };

        for (auto* s : sliders)
        {
            s->setBounds (bounds[compIdx++]);
            s->setTextBoxStyle (Slider::TextBoxBelow, false, sWidth - 10, 16);
        }

        for (auto* b : boxes)
            b->setBounds (bounds[compIdx++].withHeight (yDim * 5 / 6));

        for (auto* b : buttons)
            b->setBounds (bounds[compIdx++].withHeight (yDim).translated (0, yDim * 10 / 6));

        return;
    }

    if (totalNumComponents > 5)
    {
        auto width = (getWidth() - 10) / totalNumComponents;
        int x = proportionOfWidth (0.019f);
        const auto yPad = proportionOfHeight (0.107f);
        for (auto* s : sliders)
        {
            s->setSliderStyle (Slider::SliderStyle::LinearVertical);
            s->setTextBoxStyle (Slider::TextBoxBelow, false, width - 2, 14);
            auto bounds = Rectangle { x + 2, yPad, width - 4, getHeight() - yPad };
            x += width;

            s->setBounds (bounds);
        }
    }
}
