#include "GlobalParamControls.h"
#include "gui/GUIConstants.h"
#include "processors/chain/ChainIOProcessor.h"

namespace GUIColours = GUIConstants::Colours;

GlobalParamControls::GlobalParamControls (AudioProcessorValueTreeState& vts,
                                          const HostContextProvider& hostContextProvider,
                                          chowdsp::VariableOversampling<float>& oversampling)
    : osMenu (oversampling, vts)
{
    auto addComboBox = [this, &vts] (const String& paramTag)
    {
        auto newBox = std::make_unique<BoxWithAttachment>();
        addAndMakeVisible (newBox.get());

        auto* param = dynamic_cast<AudioParameterChoice*> (vts.getParameter (paramTag));
        if (param == nullptr)
        {
            jassertfalse;
            return;
        }

        newBox->setName (param->name);
        newBox->addItemList (param->choices, 1);
        newBox->setSelectedItemIndex (0);
        newBox->setColour (ComboBox::backgroundColourId, Colours::transparentBlack);
        newBox->setColour (ComboBox::outlineColourId, Colours::white);
        newBox->setColour (ComboBox::textColourId, GUIColours::controlTextColour);
        newBox->setColour (ComboBox::arrowColourId, Colours::white);

        newBox->attachment = std::make_unique<ComboBoxAttachment> (vts, param->paramID, *newBox);

        boxes.add (std::move (newBox));
    };

    addComboBox (GlobalParamTags::monoModeTag);

    auto addSlider = [this, &vts, &hostContextProvider] (const String& paramTag, const String& name)
    {
        const auto* param = chowdsp::ParamUtils::getParameterPointer<chowdsp::FloatParameter*> (vts, paramTag);
        auto newSlide = std::make_unique<SliderWithAttachment> (param, hostContextProvider);
        addAndMakeVisible (newSlide.get());
        newSlide->attachment = std::make_unique<SliderAttachment> (vts, paramTag, *newSlide);
        newSlide->setName (name);

        newSlide->setColour (Slider::backgroundColourId, Colours::transparentBlack);
        newSlide->setColour (Slider::textBoxTextColourId, GUIColours::controlTextColour);

        sliders.add (std::move (newSlide));
    };

    addSlider (GlobalParamTags::inGainTag, "Input");
    addSlider (GlobalParamTags::outGainTag, "Output");
    addSlider (GlobalParamTags::dryWetTag, "Dry/Wet");

    addAndMakeVisible (osMenu);
    osMenu.setColour (OSMenu::backgroundColourID, Colours::transparentBlack);
    osMenu.setColour (OSMenu::outlineColourID, Colours::white);
    osMenu.setColour (OSMenu::textColourID, GUIColours::controlTextColour);
    osMenu.setColour (OSMenu::accentColourID, GUIColours::osMenuAccentColour);
    osMenu.updateColours();
}

void GlobalParamControls::resized()
{
    const auto numComponents = boxes.size() + sliders.size() + 1;
    const auto compProportion = 1.0f / (float) numComponents;

    auto bounds = getLocalBounds();

    for (auto& box : boxes)
        box->setBounds (bounds.removeFromLeft (proportionOfWidth (compProportion)).reduced (3, 5));

    for (auto& slider : sliders)
        slider->setBounds (bounds.removeFromLeft (proportionOfWidth (compProportion)).reduced (2, 3));

    osMenu.setBounds (bounds.removeFromLeft (proportionOfWidth (compProportion)).reduced (2, 5));
}
