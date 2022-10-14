#include "CleanGain.h"
#include "../ParameterHelpers.h"

namespace
{
const String gainTag = "gain";
const String invertTag = "invert";
const String extendTag = "extend";
const String extGainTag = "gain_extended";

constexpr float negativeInfinityDB = -60.0f;

const Colour regularColour = Colours::yellow;
const Colour invertedColour = Colours::red;
} // namespace

CleanGain::CleanGain (UndoManager* um) : BaseProcessor ("Clean Gain", createParameterLayout(), um)
{
    chowdsp::ParamUtils::loadParameterPointer (gainDBParam, vts, gainTag);
    chowdsp::ParamUtils::loadParameterPointer (extGainDBParam, vts, extGainTag);
    chowdsp::ParamUtils::loadParameterPointer (invertParam, vts, invertTag);
    chowdsp::ParamUtils::loadParameterPointer (extendParam, vts, extendTag);

    addPopupMenuParameter (invertTag);
    addPopupMenuParameter (extendTag);

    uiOptions.backgroundColour = Colours::darkgrey.brighter (0.35f).withRotatedHue (0.25f);
    uiOptions.powerColour = regularColour;
    uiOptions.info.description = "Simple linear gain boost.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    vts.addParameterListener (invertTag, this);
}

ParamLayout CleanGain::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createGainDBParameter (params, gainTag, "Gain", -18.0f, 18.0f, 0.0f);

    const auto extGainToString = [] (float x)
    { return x <= negativeInfinityDB ? "-inf dB" : String (x, 2, false) + " dB"; };
    auto stringToExtGain = [] (const String& t)
    { return t.getFloatValue(); };
    emplace_param<chowdsp::FloatParameter> (params,
                                            extGainTag,
                                            "Extended Gain",
                                            createNormalisableRange (negativeInfinityDB, 30.0f, 0.0f),
                                            0.0f,
                                            extGainToString,
                                            stringToExtGain);

    emplace_param<chowdsp::BoolParameter> (params, invertTag, "Invert", false);
    emplace_param<chowdsp::BoolParameter> (params, extendTag, "Extend", false);

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
    const auto gainLinear = gainDB <= negativeInfinityDB ? 0.0f : Decibels::decibelsToGain (gainDB) * invertGain;
    gain.setGainLinear (gainLinear);

    dsp::AudioBlock<float> block { buffer };
    dsp::ProcessContextReplacing<float> context { block };
    gain.process (context);
}

bool CleanGain::getCustomComponents (OwnedArray<Component>& customComps)
{
    class GainSlider : public Slider,
                       private AudioProcessorValueTreeState::Listener
    {
    public:
        explicit GainSlider (AudioProcessorValueTreeState& vtState) : vts (vtState)
        {
            for (auto* s : { &gainSlider, &extGainSlider })
                addChildComponent (s);

            gainAttach = std::make_unique<SliderAttachment> (vts, gainTag, gainSlider);
            extGainAttach = std::make_unique<SliderAttachment> (vts, extGainTag, extGainSlider);

            Slider::setName (gainTag + "__" + extGainTag + "__");

            vts.addParameterListener (extendTag, this);
        }

        ~GainSlider() override
        {
            vts.removeParameterListener (extendTag, this);
        }

        void parameterChanged (const String& paramID, float newValue) override
        {
            if (paramID != extendTag)
                return;

            updateSliderVisibility (newValue == 1.0f);
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
            updateSliderVisibility (vts.getRawParameterValue (extendTag)->load() == 1.0f);
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
        Slider gainSlider, extGainSlider;
        std::unique_ptr<SliderAttachment> gainAttach, extGainAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainSlider)
    };

    customComps.add (std::make_unique<GainSlider> (vts));

    return false;
}

void CleanGain::parameterChanged (const String& paramID, float newValue)
{
    if (paramID != invertTag)
    {
        jassertfalse;
        return;
    }

    uiOptions.powerColour = newValue > 0.5f ? invertedColour : regularColour;
    uiOptionsChanged();
}
