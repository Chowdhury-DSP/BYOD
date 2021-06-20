#include "KnobsComponent.h"

KnobsComponent::KnobsComponent (AudioProcessorValueTreeState& vts, std::function<void()> paramLambda)
{
    auto addSlider = [=, &vts] (AudioParameterFloat* param)
    {
        auto newSlide = std::make_unique<SliderWithAttachment>();
        addAndMakeVisible (newSlide->slider);
        newSlide->attachment = std::make_unique<SliderAttachment> (vts, param->paramID, newSlide->slider);

        newSlide->slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        newSlide->slider.setName (param->name);
        newSlide->slider.setTextBoxStyle (Slider::TextBoxBelow, false, 75, 16);
        newSlide->slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        newSlide->slider.onValueChange = paramLambda;

        sliders.add (std::move (newSlide));
    };

    auto addBox = [=, &vts] (AudioParameterChoice* param)
    {
        auto newBox = std::make_unique<BoxWithAttachment>();
        addAndMakeVisible (newBox->box);
        newBox->box.setName (param->name);
        newBox->box.addItemList (param->choices, 1);
        newBox->box.setSelectedItemIndex (0);
        newBox->box.onChange = paramLambda;

        newBox->attachment = std::make_unique<ComboBoxAttachment> (vts, param->paramID, newBox->box);

        boxes.add (std::move (newBox));
    };

    auto addButton = [=, &vts] (AudioParameterBool* param)
    {
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
    g.setColour (Colours::white);
    auto makeName = [this, &g] (Component& comp, String name)
    {
        const int height = 20;
        Rectangle<int> nameBox (comp.getX(), 2, comp.getWidth(), height);
        g.drawFittedText (name, nameBox, Justification::centred, 1);
    };

    for (auto* s : sliders)
        makeName (s->slider, s->slider.getName());

    for (auto* b : boxes)
        makeName (b->box, b->box.getName());
}

void KnobsComponent::resized()
{
    int totalNumComponents = sliders.size() + boxes.size() + buttons.size();

    int compHeight = getHeight() - 20;
    int compWidth = totalNumComponents > 1 ? (getWidth() - 10) / totalNumComponents : compHeight;

    int x = 5;
    bool first = true;
    for (auto* s : sliders)
    {
        int offset = first ? 0 : 20;
        s->slider.setBounds (x - offset, 15, compWidth, compWidth);
        x = s->slider.getRight() + 5;
        first = false;
    }

    for (auto* b : boxes)
    {
        int offset = first ? 0 : 5;
        b->box.setBounds (x - offset, 15 + (compHeight - 30) / 2, compWidth - 5, 30);
        x = b->box.getRight() + 10;
        first = false;
    }

    for (auto* b : buttons)
    {
        int offset = first ? 0 : 5;
        b->button.setBounds (x - offset, 15 + (compHeight - 30) / 2, compWidth - 5, 30);
        x = b->button.getRight() + 10;
        first = false;
    }
}
