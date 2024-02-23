#include "KnobsComponent.h"
#include "gui/utils/LookAndFeels.h"
#include "processors/BaseProcessor.h"

namespace KnobDims
{
constexpr float nameHeightScale = 0.115f;
}

KnobsComponent::KnobsComponent (BaseProcessor& baseProc,
                                AudioProcessorValueTreeState& vts,
                                const Colour& cc,
                                const Colour& ac,
                                chowdsp::HostContextProvider& hostContextProvider)
{
    if (auto* lnf = baseProc.getCustomLookAndFeel())
        setLookAndFeel (lnf);

    const auto addSlider = [this,
                            &vts,
                            &hostContextProvider] (chowdsp::FloatParameter* param)
    {
        auto newSlide = [this, &param, &hostContextProvider]
        {
            if (const auto* lnf = dynamic_cast<ProcessorLNF*> (&getLookAndFeel()))
                return lnf->createSlider (*param, hostContextProvider);
            return std::make_unique<ModulatableSlider> (*param, hostContextProvider);
        }();

        addAndMakeVisible (newSlide.get());
        newSlide->attachment = std::make_unique<ModulatableSlider::SliderAttachment> (vts, param->paramID, *newSlide);
        hostContextProvider.registerParameterComponent (*newSlide, *param);

        newSlide->setComponentID (param->paramID);
        newSlide->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        newSlide->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        newSlide->setName (param->name);
        newSlide->setTextBoxStyle (Slider::TextBoxBelow, false, 80, 20);
        sliders.add (std::move (newSlide));
    };

    const auto addBox = [this, &vts, &hostContextProvider] (AudioParameterChoice* param)
    {
        auto newBox = std::make_unique<BoxWithAttachment>();
        hostContextProvider.registerParameterComponent (*newBox, *param);
        addAndMakeVisible (newBox.get());
        newBox->setComponentID (param->paramID);
        newBox->setName (param->name);
        newBox->addItemList (param->choices, 1);
        newBox->setSelectedItemIndex (0);
        newBox->setScrollWheelEnabled (true);
        newBox->attachment = std::make_unique<ComboBoxAttachment> (vts, param->paramID, *newBox);
        boxes.add (std::move (newBox));
    };

    const auto addButton = [this, &vts, &hostContextProvider] (AudioParameterBool* param)
    {
        if (param->paramID == "on_off")
            return;

        auto newButton = std::make_unique<ButtonWithAttachment>();
        hostContextProvider.registerParameterComponent (*newButton, *param);
        addAndMakeVisible (newButton.get());
        newButton->setComponentID (param->paramID);
        newButton->setButtonText (param->name);
        newButton->setClickingTogglesState (true);
        newButton->attachment = std::make_unique<ButtonAttachment> (vts, param->paramID, *newButton);
        buttons.add (std::move (newButton));
    };

    auto customComponentsFirst = baseProc.getCustomComponents (customComponents, hostContextProvider);

    for (auto* param : vts.processor.getParameters())
    {
        if (auto* rangedParam = dynamic_cast<RangedAudioParameter*> (param))
        {
            if (baseProc.getUIOptions().paramIDsToSkip.contains (rangedParam->paramID))
                continue;

            bool found = false;
            for (auto* comp : customComponents)
            {
                if (comp->getName().contains (rangedParam->paramID + "__"))
                {
                    found = true;
                    break;
                }
            }

            if (found)
                continue;
        }

        if (auto* paramFloat = dynamic_cast<chowdsp::FloatParameter*> (param))
            addSlider (paramFloat);
        else if (auto* paramChoice = dynamic_cast<AudioParameterChoice*> (param))
            addBox (paramChoice);
        else if (auto* paramBool = dynamic_cast<AudioParameterBool*> (param))
            addButton (paramBool);
        else
            jassertfalse; // unknown parameter type
    }

    for (int i = customComponents.size() - 1; i >= 0; --i)
    {
        auto* comp = customComponents[i];

        addAndMakeVisible (comp);
        if (auto* sliderComp = dynamic_cast<Slider*> (comp))
        {
            sliderComp->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
            customComponents.removeObject (comp, false);
            if (customComponentsFirst)
                sliders.insert (0, sliderComp);
            else
                sliders.add (sliderComp);
        }
        else if (auto* boxComp = dynamic_cast<ComboBox*> (comp))
        {
            customComponents.removeObject (comp, false);
            if (customComponentsFirst)
                boxes.insert (0, boxComp);
            else
                boxes.add (boxComp);
        }
    }

    setColours (cc, ac);
    setSize (getWidth(), 100);
}

void KnobsComponent::setColours (const Colour& cc, const Colour& ac)
{
    contrastColour = cc;
    accentColour = ac;

    for (auto* slider : sliders)
    {
        slider->setColour (Slider::textBoxOutlineColourId, contrastColour);
        slider->setColour (Slider::textBoxTextColourId, contrastColour);
        slider->setColour (Slider::textBoxBackgroundColourId, Colours::transparentWhite);
        slider->setColour (Slider::textBoxHighlightColourId, accentColour.withAlpha (0.55f));
        slider->setColour (Slider::thumbColourId, accentColour);
    }

    for (auto* box : boxes)
    {
        box->setColour (ComboBox::outlineColourId, contrastColour);
        box->setColour (ComboBox::textColourId, contrastColour);
        box->setColour (ComboBox::arrowColourId, contrastColour);
    }

    for (auto* button : buttons)
    {
        button->setColour (TextButton::buttonColourId, contrastColour.withAlpha (0.4f));
        button->setColour (TextButton::buttonOnColourId, accentColour);
        button->setColour (TextButton::textColourOnId, contrastColour);
    }

    for (auto* comp : customComponents)
    {
        if (auto* sliderComp = dynamic_cast<Slider*> (comp))
        {
            sliderComp->setColour (Slider::textBoxOutlineColourId, contrastColour);
            sliderComp->setColour (Slider::textBoxTextColourId, contrastColour);
            sliderComp->setColour (Slider::textBoxBackgroundColourId, Colours::transparentWhite);
            sliderComp->setColour (Slider::textBoxHighlightColourId, accentColour.withAlpha (0.55f));
            sliderComp->setColour (Slider::thumbColourId, accentColour);
        }
        else if (auto* boxComp = dynamic_cast<ComboBox*> (comp))
        {
            boxComp->setColour (ComboBox::outlineColourId, contrastColour);
            boxComp->setColour (ComboBox::textColourId, contrastColour);
            boxComp->setColour (ComboBox::arrowColourId, contrastColour);
        }
    }

    repaint();
}

void KnobsComponent::toggleParamsEnabled (const std::vector<String>& paramIDs, bool shouldEnable)
{
    for (auto* comp : getChildren())
    {
        if (std::find (paramIDs.begin(), paramIDs.end(), comp->getComponentID()) != paramIDs.end())
            comp->setEnabled (shouldEnable);
    }
}

void KnobsComponent::paint (Graphics& g)
{
    g.setColour (contrastColour.withAlpha (isEnabled() ? 1.0f : 0.6f));

    const auto nameHeight = proportionOfHeight (KnobDims::nameHeightScale);
    const auto nameOffset = proportionOfHeight (0.157f);
    auto makeName = [&g, nameHeight, nameOffset] (Component& comp, const String& name, int offset = 0)
    {
        const auto textWidth = comp.proportionOfWidth (0.8f);
        if (textWidth == 0)
            return;

        auto font = Font ((float) nameHeight - 2.0f).boldened();
        while (font.getStringWidth (name) > textWidth)
            font = Font (font.getHeight() - 0.5f).boldened();

        g.setFont (font);
        juce::Rectangle<int> nameBox (comp.getX(), comp.getY() - nameOffset + offset, comp.getWidth(), nameHeight);
        g.drawFittedText (name, nameBox, Justification::centred, 1);
    };

    for (auto* s : sliders)
        makeName (*s, s->getName(), 6);

    for (auto* b : boxes)
        makeName (*b, b->getName());
}

void KnobsComponent::resized()
{
    int totalNumComponents = sliders.size() + boxes.size() + buttons.size() + customComponents.size();

    const auto nameHeight = proportionOfHeight (KnobDims::nameHeightScale);
    int compHeight = getHeight() - nameHeight;
    int compWidth = totalNumComponents > 1 ? (getWidth() - 10) / totalNumComponents : compHeight;

    if (totalNumComponents == 1)
    {
        const int x = (getWidth() - compWidth) / 2;
        const auto yPad = proportionOfHeight (0.107f);
        const auto yDim = proportionOfHeight (0.214f);

        if (sliders.size() > 0)
        {
            sliders[0]->setTextBoxStyle (Slider::TextBoxBelow, false, compWidth * 4 / 5, proportionOfHeight (0.137f));
            sliders[0]->setBounds (x, yPad, compWidth - 5, compWidth - 5);
        }

        if (boxes.size() > 0)
        {
            compWidth = getWidth() - yDim;
            boxes[0]->setBounds (juce::Rectangle (compWidth, yDim).withCentre (getLocalBounds().getCentre()));
        }

        if (buttons.size() > 0)
            buttons[0]->setBounds (x, yPad + (compHeight - yDim) / 2, compWidth - 5, yDim);

        if (customComponents.size() > 0)
            customComponents[0]->setBounds (getLocalBounds());

        return;
    }

    if (totalNumComponents == 2)
    {
        int compIdx = 0;
        const int x = (getWidth() - 2 * compWidth) / 2;
        const auto yPad = proportionOfHeight (0.107f);
        const auto yDim = proportionOfHeight (0.214f);

        for (auto* s : sliders)
        {
            s->setTextBoxStyle (Slider::TextBoxBelow, false, compWidth * 4 / 5, proportionOfHeight (0.137f));
            s->setBounds (x + (compIdx++) * compWidth, yPad, compWidth, compWidth);
        }

        for (auto* b : boxes)
            b->setBounds (x + (compIdx++) * compWidth, yPad + (compHeight - yDim) / 2, compWidth - 5, yDim);

        for (auto* b : buttons)
            b->setBounds (x + (compIdx++) * compWidth, yPad + (compHeight - yDim) / 2, compWidth - 5, yDim);

        for (auto* c : customComponents)
            c->setBounds (x + (compIdx++) * compWidth, 0, compWidth, getHeight());

        return;
    }

    if (totalNumComponents == 3)
    {
        int compIdx = 0;
        const int x = (getWidth() - 3 * compWidth) / 2;
        const auto yDim = proportionOfHeight (0.214f);

        for (auto* s : sliders)
        {
            s->setTextBoxStyle (Slider::TextBoxBelow, false, compWidth * 4 / 5, proportionOfHeight (0.137f));
            s->setBounds (x + (compIdx++) * compWidth, yDim, compWidth, compWidth);
        }

        for (auto* b : boxes)
            b->setBounds (x + (compIdx++) * compWidth, yDim + (compHeight - 2 * yDim) / 2, compWidth - 5, yDim);

        for (auto* b : buttons)
            b->setBounds (x + (compIdx++) * compWidth, yDim + (compHeight - 2 * yDim) / 2, compWidth - 5, yDim);

        return;
    }

    if (totalNumComponents == 4)
    {
        int compIdx = 0;

        juce::Rectangle<int> bounds[4];
        const int sWidth = proportionOfWidth (0.308f);
        const int xOff = proportionOfWidth (0.231f);
        const int yOff = proportionOfHeight (0.143f);
        bounds[0] = juce::Rectangle { 0, yOff, sWidth, sWidth };
        bounds[1] = juce::Rectangle { xOff, 2 * yOff, sWidth, sWidth };
        bounds[2] = juce::Rectangle { 2 * xOff, yOff, sWidth, sWidth };
        bounds[3] = juce::Rectangle { 3 * xOff, 2 * yOff, sWidth, sWidth };

        for (auto* s : sliders)
        {
            s->setTextBoxStyle (Slider::TextBoxBelow, false, sWidth - 15, proportionOfHeight (0.123f));
            s->setBounds (bounds[compIdx++]);
        }

        const auto yDim = proportionOfHeight (0.214f);
        for (auto* b : boxes)
            b->setBounds (bounds[compIdx++].withHeight (yDim).translated (-5, 2 * yDim));

        for (auto* b : buttons)
            b->setBounds (bounds[compIdx++].withHeight (yDim).translated (0, yDim * 10 / 6));

        return;
    }

    if (totalNumComponents == 5 && sliders.size() == 5)
    {
        int compIdx = 0;

        juce::Rectangle<int> bounds[5];
        const int sWidth = proportionOfWidth (0.215f);
        const int y1 = proportionOfHeight (0.25f);
        const int y2 = proportionOfHeight (0.60f);
        const int y3 = proportionOfHeight (0.11f);
        const int x1 = proportionOfHeight (0.02f);
        const int x2 = proportionOfWidth (0.21f);
        const int x3 = proportionOfWidth (0.56f);
        const int x4 = proportionOfWidth (0.75f);
        const int x5 = proportionOfWidth (0.39f);

        bounds[0] = juce::Rectangle { x1, y1, sWidth, sWidth };
        bounds[1] = juce::Rectangle { x2, y2, sWidth, sWidth };
        bounds[2] = juce::Rectangle { x3, y2, sWidth, sWidth };
        bounds[3] = juce::Rectangle { x4, y1, sWidth, sWidth };
        bounds[4] = juce::Rectangle { x5, y3, sWidth, sWidth };

        for (auto* s : sliders)
        {
            s->setTextBoxStyle (Slider::TextBoxBelow, false, sWidth - 10, proportionOfHeight (0.123f));
            s->setBounds (bounds[compIdx++]);
        }

        return;
    }

    if (totalNumComponents == 5)
    {
        int compIdx = 0;

        juce::Rectangle<int> bounds[5];
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

        bounds[0] = juce::Rectangle { 0, y1, sWidth, sWidth };
        bounds[1] = juce::Rectangle { x1, y2, sWidth, sWidth };
        bounds[2] = juce::Rectangle { x2, y1, sWidth, sWidth };
        bounds[3] = juce::Rectangle { x3, y1, sWidth, sWidth };
        bounds[4] = juce::Rectangle { x4, y3, xDim, yDim };

        for (auto* s : sliders)
        {
            s->setTextBoxStyle (Slider::TextBoxBelow, false, sWidth - 10, proportionOfHeight (0.123f));
            s->setBounds (bounds[compIdx++]);
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
            s->setTextBoxStyle (Slider::TextBoxBelow, false, width - 2, proportionOfHeight (0.11f));
            auto bounds = juce::Rectangle { x + 2, yPad, width - 4, getHeight() - yPad };
            x += width;

            s->setBounds (bounds);
        }
    }
}
