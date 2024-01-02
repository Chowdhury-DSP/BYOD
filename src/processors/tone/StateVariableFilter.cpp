#include "StateVariableFilter.h"
#include "../ParameterHelpers.h"
#include "gui/utils/ModulatableSlider.h"

namespace SVFTags
{
const String modeTag = "mode";
const String multiModeTag = "multi_mode";
const String multiModeTypeTag = "multi_mode_type";
} // namespace SVFTags

StateVariableFilter::StateVariableFilter (UndoManager* um) : BaseProcessor ("SVF", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (freqParam, vts, "freq");
    loadParameterPointer (qParam, vts, "q_value");
    modeParam = vts.getRawParameterValue (SVFTags::modeTag);
    multiModeOnOffParam = vts.getRawParameterValue (SVFTags::multiModeTag);
    loadParameterPointer (multiModeParam, vts, SVFTags::multiModeTypeTag);

    addPopupMenuParameter (SVFTags::multiModeTag);

    uiOptions.backgroundColour = Colours::blanchedalmond;
    uiOptions.powerColour = Colours::red.darker (0.25f);
    uiOptions.info.description = "A state variable filter, with lowpass, highpass, and bandpass modes.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout StateVariableFilter::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createFreqParameter (params, "freq", "Freq.", 20.0f, 20000.0f, 2000.0f, 8000.0f);

    auto qRange = createNormalisableRange (0.2f, 5.0f, 0.7071f);
    emplace_param<chowdsp::FloatParameter> (params,
                                            "q_value",
                                            "Q",
                                            qRange,
                                            0.7071f,
                                            &floatValToString,
                                            &stringToFloatVal);

    emplace_param<AudioParameterChoice> (params,
                                         SVFTags::modeTag,
                                         "Mode",
                                         StringArray { "LPF", "HPF", "BPF" },
                                         0);

    emplace_param<AudioParameterBool> (params, SVFTags::multiModeTag, "Multi-Mode", true);
    createPercentParameter (params, SVFTags::multiModeTypeTag, "Mode", 0.0f);

    return { params.begin(), params.end() };
}

void StateVariableFilter::prepare (double sampleRate, int samplesPerBlock)
{
    svf.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });

    freqSmooth.reset (sampleRate, 0.01);
    freqSmooth.setCurrentAndTargetValue (*freqParam);

    modeSmoothed.prepare (sampleRate, samplesPerBlock);
    modeSmoothed.setRampLength (0.025);
}

void StateVariableFilter::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    freqSmooth.setTargetValue (*freqParam);
    svf.setQValue (*qParam);

    float modeValue = 0.0f;
    if (*multiModeOnOffParam == 1.0f)
    {
        modeValue = multiModeParam->getCurrentValue();
    }
    else
    {
        auto fixedMode = (int) *modeParam;
        if (fixedMode == 0) // lowpass
            modeValue = 0.0f;
        else if (fixedMode == 1) // highpass
            modeValue = 1.0f;
        else if (fixedMode == 2) // bandpass
            modeValue = 0.5f;
    }
    modeSmoothed.process (modeValue, buffer.getNumSamples());

    if (freqSmooth.isSmoothing() || modeSmoothed.isSmoothing())
    {
        const auto smoothedModeData = modeSmoothed.getSmoothedBuffer();
        if (numChannels == 1)
        {
            auto* x = buffer.getWritePointer (0);
            for (int n = 0; n < numSamples; ++n)
            {
                svf.setCutoffFrequency (freqSmooth.getNextValue());
                svf.setMode (smoothedModeData[n]);
                x[n] = svf.processSample (0, x[n]);
            }
        }
        else if (numChannels == 2)
        {
            auto* left = buffer.getWritePointer (0);
            auto* right = buffer.getWritePointer (1);
            for (int n = 0; n < numSamples; ++n)
            {
                svf.setCutoffFrequency (freqSmooth.getNextValue());
                svf.setMode (smoothedModeData[n]);
                left[n] = svf.processSample (0, left[n]);
                right[n] = svf.processSample (1, right[n]);
            }
        }
    }
    else
    {
        auto block = dsp::AudioBlock<float> { buffer };
        auto context = dsp::ProcessContextReplacing<float> { block };
        svf.setCutoffFrequency (freqSmooth.getNextValue());
        svf.setMode (modeSmoothed.getCurrentValue());
        svf.process (context);
    }
}

void StateVariableFilter::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);

    using namespace std::string_view_literals;
    if (version <= chowdsp::Version { "1.0.1"sv })
    {
        // Multi-mode behaviour was only added in version 1.0.2, so we need to
        // make sure we don't break older patches.
        vts.getParameter (SVFTags::multiModeTag)->setValueNotifyingHost (0.0f);
    }
}

bool StateVariableFilter::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;
    class ModeControl : public Slider
    {
    public:
        ModeControl (AudioProcessorValueTreeState& vtState, chowdsp::HostContextProvider& hcp)
            : vts (vtState),
              modeSelectorAttach (vts, SVFTags::modeTag, modeSelector),
              multiModeSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, SVFTags::multiModeTypeTag), hcp),
              multiModeAttach (vts, SVFTags::multiModeTypeTag, multiModeSlider),
              multiModeOnOffAttach (
                  *vts.getParameter (SVFTags::multiModeTag),
                  [this] (float newValue)
                  { updateControlVisibility (newValue == 1.0f); },
                  vts.undoManager)
        {
            addChildComponent (modeSelector);
            addChildComponent (multiModeSlider);

            const auto* modeChoiceParam = getParameterPointer<AudioParameterChoice*> (vts, SVFTags::modeTag);
            modeSelector.addItemList (modeChoiceParam->choices, 1);
            modeSelector.setSelectedItemIndex (0);
            modeSelector.setScrollWheelEnabled (true);
            hcp.registerParameterComponent (modeSelector, *modeChoiceParam);

            hcp.registerParameterComponent (multiModeSlider, multiModeSlider.getParameter());

            Component::setName (SVFTags::modeTag + "__" + SVFTags::multiModeTypeTag + "__");
        }

        void colourChanged() override
        {
            for (auto colourID : { Slider::textBoxOutlineColourId,
                                   Slider::textBoxTextColourId,
                                   Slider::textBoxBackgroundColourId,
                                   Slider::textBoxHighlightColourId,
                                   Slider::thumbColourId })
            {
                multiModeSlider.setColour (colourID, findColour (colourID, false));
            }

            for (auto colourID : { ComboBox::outlineColourId,
                                   ComboBox::textColourId,
                                   ComboBox::arrowColourId })
            {
                modeSelector.setColour (colourID, findColour (Slider::textBoxTextColourId, false));
            }
        }

        void updateControlVisibility (bool multiModeOn)
        {
            modeSelector.setVisible (! multiModeOn);
            multiModeSlider.setVisible (multiModeOn);

            setName (vts.getParameter (multiModeOn ? SVFTags::multiModeTypeTag : SVFTags::modeTag)->name);
        }

        void visibilityChanged() override
        {
            updateControlVisibility (vts.getRawParameterValue (SVFTags::multiModeTag)->load() == 1.0f);
        }

        void resized() override
        {
            multiModeSlider.setSliderStyle (getSliderStyle());
            multiModeSlider.setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());

            const auto bounds = getLocalBounds();
            modeSelector.setBounds (bounds.withHeight (proportionOfHeight (0.4f)));
            multiModeSlider.setBounds (bounds);
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
        using BoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;

        AudioProcessorValueTreeState& vts;

        ComboBox modeSelector;
        BoxAttachment modeSelectorAttach;

        ModulatableSlider multiModeSlider;
        SliderAttachment multiModeAttach;

        ParameterAttachment multiModeOnOffAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeControl)
    };

    customComps.add (std::make_unique<ModeControl> (vts, hcp));

    return false;
}
