#include "StateVariableFilter.h"
#include "../ParameterHelpers.h"

namespace
{
const String modeTag = "mode";
const String multiModeTag = "multi_mode";
const String multiModeTypeTag = "multi_mode_type";
} // namespace

StateVariableFilter::StateVariableFilter (UndoManager* um) : BaseProcessor ("SVF", createParameterLayout(), um)
{
    freqParam = vts.getRawParameterValue ("freq");
    qParam = vts.getRawParameterValue ("q_value");
    modeParam = vts.getRawParameterValue (modeTag);
    multiModeOnOffParam = vts.getRawParameterValue (multiModeTag);
    multiModeParam = vts.getRawParameterValue (multiModeTypeTag);

    addPopupMenuParameter (multiModeTag);

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
                                         modeTag,
                                         "Mode",
                                         StringArray { "LPF", "HPF", "BPF" },
                                         0);

    // @TODO: for compatibility with version 1 we need this to be off by default,
    // but when we add version streaming we should make this true by default for new versions
    emplace_param<AudioParameterBool> (params, multiModeTag, "Multi-Mode", false);
    createPercentParameter (params, multiModeTypeTag, "Mode", 0.0f);

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
        modeValue = multiModeParam->load();
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

bool StateVariableFilter::getCustomComponents (OwnedArray<Component>& customComps)
{
    class ModeControl : public Slider,
                        private AudioProcessorValueTreeState::Listener
    {
    public:
        explicit ModeControl (AudioProcessorValueTreeState& vtState) : vts (vtState)
        {
            addChildComponent (modeSelector);
            addChildComponent (multiModeSlider);

            modeSelectorAttach = std::make_unique<BoxAttachment> (vts, modeTag, modeSelector);
            multiModeAttach = std::make_unique<SliderAttachment> (vts, multiModeTypeTag, multiModeSlider);

            modeSelector.addItemList (dynamic_cast<AudioParameterChoice*> (vts.getParameter (modeTag))->choices, 1);
            modeSelector.setSelectedItemIndex (0);

            setName (modeTag + "__" + multiModeTypeTag + "__");

            vts.addParameterListener (multiModeTag, this);
        }

        ~ModeControl() override
        {
            vts.removeParameterListener (multiModeTag, this);
        }

        void parameterChanged (const String& paramID, float newValue) override
        {
            if (paramID != multiModeTag)
                return;

            updateControlVisibility (newValue == 1.0f);
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

            setName (vts.getParameter (multiModeOn ? multiModeTypeTag : modeTag)->name);
        }

        void visibilityChanged() override
        {
            updateControlVisibility (vts.getRawParameterValue (multiModeTag)->load() == 1.0f);
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
        std::unique_ptr<BoxAttachment> modeSelectorAttach;

        Slider multiModeSlider;
        std::unique_ptr<SliderAttachment> multiModeAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeControl)
    };

    customComps.add (std::make_unique<ModeControl> (vts));

    return false;
}
