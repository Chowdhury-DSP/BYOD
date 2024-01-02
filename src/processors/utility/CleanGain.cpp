#include "CleanGain.h"
#include "../ParameterHelpers.h"
#include "gui/utils/ModulatableSlider.h"

namespace GainTags
{
const String gainTag = "gain";
const String invertTag = "invert";
const String extendTag = "extend";
const String extGainTag = "gain_extended";

constexpr float negativeInfinityDB = -60.0f;

const Colour regularColour = Colours::yellow;
const Colour invertedColour = Colours::red;
} // namespace GainTags

CleanGain::CleanGain (UndoManager* um) : BaseProcessor ("Clean Gain", createParameterLayout(), um),
                                         invertAttach (
                                             *vts.getParameter (GainTags::invertTag),
                                             [this] (float newValue)
                                             {
                                                 uiOptions.powerColour = newValue > 0.5f ? GainTags::invertedColour : GainTags::regularColour;
                                                 uiOptionsChanged();
                                             },
                                             um)
{
    chowdsp::ParamUtils::loadParameterPointer (gainDBParam, vts, GainTags::gainTag);
    chowdsp::ParamUtils::loadParameterPointer (extGainDBParam, vts, GainTags::extGainTag);
    chowdsp::ParamUtils::loadParameterPointer (invertParam, vts, GainTags::invertTag);
    chowdsp::ParamUtils::loadParameterPointer (extendParam, vts, GainTags::extendTag);

    addPopupMenuParameter (GainTags::invertTag);
    addPopupMenuParameter (GainTags::extendTag);

    uiOptions.backgroundColour = Colours::darkgrey.brighter (0.35f).withRotatedHue (0.25f);
    uiOptions.powerColour = GainTags::regularColour;
    uiOptions.info.description = "Simple linear gain boost.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout CleanGain::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createGainDBParameter (params, GainTags::gainTag, "Gain", -18.0f, 18.0f, 0.0f);

    const auto extGainToString = [] (float x)
    { return x <= GainTags::negativeInfinityDB ? "-inf dB" : String (x, 2, false) + " dB"; };
    auto stringToExtGain = [] (const String& t)
    { return t.getFloatValue(); };
    emplace_param<chowdsp::FloatParameter> (params,
                                            GainTags::extGainTag,
                                            "Extended Gain",
                                            createNormalisableRange (GainTags::negativeInfinityDB, 30.0f, 0.0f),
                                            0.0f,
                                            extGainToString,
                                            stringToExtGain);

    emplace_param<chowdsp::BoolParameter> (params, GainTags::invertTag, "Invert", false);
    emplace_param<chowdsp::BoolParameter> (params, GainTags::extendTag, "Extend", false);

    return { params.begin(), params.end() };
}

void CleanGain::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    gain.prepare (spec);
    gain.setRampDurationSeconds (0.01);
}

void CleanGain::processAudio (AudioBuffer<float>& buffer)
{
    const auto invertGain = invertParam->get() ? -1.0f : 1.0f;
    const auto gainDB = extendParam->get() ? extGainDBParam->getCurrentValue() : gainDBParam->getCurrentValue();
    const auto gainLinear = gainDB <= GainTags::negativeInfinityDB ? 0.0f : Decibels::decibelsToGain (gainDB) * invertGain;
    gain.setGainLinear (gainLinear);

    dsp::AudioBlock<float> block { buffer };
    dsp::ProcessContextReplacing<float> context { block };
    gain.process (context);
}

bool CleanGain::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;
    class GainSlider : public Slider
    {
    public:
        GainSlider (AudioProcessorValueTreeState& vtState, chowdsp::HostContextProvider& hcp)
            : vts (vtState),
              gainSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, GainTags::gainTag), hcp),
              extGainSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, GainTags::extGainTag), hcp),
              gainAttach (vts, GainTags::gainTag, gainSlider),
              extGainAttach (vts, GainTags::extGainTag, extGainSlider),
              extendAttach (
                  *vts.getParameter (GainTags::extendTag),
                  [this] (float newValue)
                  { updateSliderVisibility (newValue == 1.0f); },
                  vts.undoManager)
        {
            for (auto* s : { &gainSlider, &extGainSlider })
                addChildComponent (s);

            hcp.registerParameterComponent (gainSlider, gainSlider.getParameter());
            hcp.registerParameterComponent (extGainSlider, extGainSlider.getParameter());

            Slider::setName (GainTags::gainTag + "__" + GainTags::extGainTag + "__");
        }

        void colourChanged() override
        {
            for (auto* s : { &gainSlider, &extGainSlider })
            {
                for (auto colourID : { Slider::textBoxOutlineColourId,
                                       Slider::textBoxTextColourId,
                                       Slider::textBoxBackgroundColourId,
                                       Slider::textBoxHighlightColourId,
                                       Slider::thumbColourId })
                {
                    s->setColour (colourID, findColour (colourID, false));
                }
            }
        }

        void updateSliderVisibility (bool extendedGainOn)
        {
            gainSlider.setVisible (! extendedGainOn);
            extGainSlider.setVisible (extendedGainOn);

            setName ("Gain");
        }

        void visibilityChanged() override
        {
            updateSliderVisibility (vts.getRawParameterValue (GainTags::extendTag)->load() == 1.0f);
        }

        void resized() override
        {
            for (auto* s : { &gainSlider, &extGainSlider })
            {
                s->setSliderStyle (getSliderStyle());
                s->setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());
            }

            gainSlider.setBounds (getLocalBounds());
            extGainSlider.setBounds (getLocalBounds());
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

        AudioProcessorValueTreeState& vts;
        ModulatableSlider gainSlider, extGainSlider;
        SliderAttachment gainAttach, extGainAttach;
        ParameterAttachment extendAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainSlider)
    };

    customComps.add (std::make_unique<GainSlider> (vts, hcp));

    return false;
}
