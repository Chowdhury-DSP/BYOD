#include "EnvelopeFilter.h"
#include "../ParameterHelpers.h"

namespace
{
constexpr int mSize = 16;

constexpr float highQ = 5.0f;
constexpr float lowQ = 1.0f / MathConstants<float>::sqrt2;
float getQ (float param01)
{
    return lowQ * std::pow ((highQ / lowQ), param01);
}

const auto speedRange = ParameterHelpers::createNormalisableRange (5.0f, 100.0f, 20.0f);

const String senseTag = "sense";
const String directControlTag = "direct_control";
const String freqModTag = "freq_mod";
} // namespace

EnvelopeFilter::EnvelopeFilter (UndoManager* um) : BaseProcessor ("Envelope Filter", createParameterLayout(), um)
{
    freqParam = vts.getRawParameterValue ("freq");
    resParam = vts.getRawParameterValue ("res");
    speedParam = vts.getRawParameterValue ("speed");
    senseParam = vts.getRawParameterValue (senseTag);
    freqModParam = vts.getRawParameterValue (freqModTag);
    filterTypeParam = vts.getRawParameterValue ("filter_type");
    directControlParam = vts.getRawParameterValue (directControlTag);

    uiOptions.backgroundColour = Colours::purple.brighter();
    uiOptions.powerColour = Colours::yellow.darker (0.1f);
    uiOptions.info.description = "A envelope filter with lowpass, bandpass, and highpass filter types.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout EnvelopeFilter::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "res", "Resonance", 0.5f);
    createFreqParameter (params, "freq", "Freq.", 100.0f, 1000.0f, 250.0f, 250.0f);
    createPercentParameter (params, "speed", "Speed", 0.5f);
    createPercentParameter (params, senseTag, "Sensitivity", 0.5f);
    createPercentParameter (params, freqModTag, "Freq. Mod", 0.0f);

    emplace_param<AudioParameterChoice> (params, "filter_type", "Type", StringArray { "Lowpass", "Bandpass", "Highpass" }, 0);

    emplace_param<AudioParameterBool> (params, directControlTag, "Direct Control", false);

    return { params.begin(), params.end() };
}

void EnvelopeFilter::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    auto monoSpec = spec;
    monoSpec.numChannels = 1;

    filter.prepare (spec);
    level.prepare (monoSpec);

    levelBuffer.setSize (1, samplesPerBlock);
}

void EnvelopeFilter::fillLevelBuffer (AudioBuffer<float>& buffer, bool directControlOn)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    levelBuffer.setSize (1, numSamples, false, false, true);
    levelBuffer.clear();

    if (directControlOn)
    {
        FloatVectorOperations::fill (levelBuffer.getWritePointer (0), *freqModParam, numSamples);
    }
    else if (numChannels == 1)
    {
        levelBuffer.copyFrom (0, 0, buffer.getReadPointer (0), numSamples);
    }
    else
    {
        levelBuffer.copyFrom (0, 0, buffer.getReadPointer (0), numSamples);

        for (int ch = 1; ch < numChannels; ++ch)
            levelBuffer.addFrom (0, 0, buffer.getReadPointer (ch), numSamples);

        levelBuffer.applyGain (1.0f / (float) numChannels);
    }

    auto speed = speedRange.convertFrom0to1 (1.0f - *speedParam);
    level.setParameters (speed, speed * 4.0f);
    dsp::AudioBlock<float> levelBlock { levelBuffer };
    dsp::ProcessContextReplacing<float> levelCtx { levelBlock };
    level.process (levelCtx);
}

template <chowdsp::StateVariableFilterType FilterType, typename ModFreqFunc>
void processFilter (AudioBuffer<float>& buffer, chowdsp::StateVariableFilter<float>& filter, ModFreqFunc&& getModFreq)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        int i = 0;
        for (int n = 0; n < numSamples - mSize; n += mSize)
        {
            filter.setCutoffFrequency (getModFreq (i));
            for (; i < n + mSize; ++i)
                x[i] = filter.processSample<FilterType> (ch, x[i]);
        }

        // process leftover samples
        filter.setCutoffFrequency (getModFreq (i));
        for (; i < numSamples; ++i)
            x[i] = filter.processSample<FilterType> (ch, x[i]);
    }
}

void EnvelopeFilter::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    bool directControlOn = *directControlParam == 1.0f;
    fillLevelBuffer (buffer, directControlOn);

    auto filterFreqHz = freqParam->load();
    filter.setResonance (getQ (resParam->load()));

    auto freqModGain = directControlOn ? 10.0f : (20.0f * senseParam->load());
    auto* levelPtr = levelBuffer.getReadPointer (0);
    FloatVectorOperations::clip (levelBuffer.getWritePointer (0), levelPtr, 0.0f, 2.0f, numSamples);

    auto getModFreq = [=] (int i) -> float
    {
        return jlimit (20.0f, 20.0e3f, filterFreqHz + freqModGain * levelPtr[i] * filterFreqHz);
    };

    using SVFType = chowdsp::StateVariableFilterType;
    auto filterType = (int) filterTypeParam->load();
    switch (filterType)
    {
        case 0:
            processFilter<SVFType::Lowpass> (buffer, filter, getModFreq);
            break;

        case 1:
            processFilter<SVFType::Bandpass> (buffer, filter, getModFreq);
            break;

        case 2:
            processFilter<SVFType::Highpass> (buffer, filter, getModFreq);
            break;

        default:
            jassertfalse;
    }

    buffer.applyGain (Decibels::decibelsToGain (-6.0f));
}

void EnvelopeFilter::addToPopupMenu (PopupMenu& menu)
{
    directControlAttach = std::make_unique<ParameterAttachment> (
        *vts.getParameter ("direct_control"), [=] (float) {}, vts.undoManager);

    PopupMenu::Item directControlItem;
    directControlItem.itemID = 1;
    directControlItem.text = "Direct Control";
    directControlItem.action = [=]
    { directControlAttach->setValueAsCompleteGesture (1.0f - *directControlParam); };
    directControlItem.colour = *directControlParam == 1.0f ? uiOptions.powerColour : Colours::white;

    menu.addItem (directControlItem);
}

void EnvelopeFilter::getCustomComponents (OwnedArray<Component>& customComps)
{
    class ControlSlider : public Slider,
                          private AudioProcessorValueTreeState::Listener
    {
    public:
        explicit ControlSlider (AudioProcessorValueTreeState& vtState) : vts (vtState)
        {
            for (auto* s : { &freqModSlider, &sensitivitySlider })
                addChildComponent (s);

            freqModAttach = std::make_unique<SliderAttachment> (vts, freqModTag, freqModSlider);
            senseAttach = std::make_unique<SliderAttachment> (vts, senseTag, sensitivitySlider);

            setName (senseTag + "__" + freqModTag + "__" + directControlTag + "__");

            vts.addParameterListener (directControlTag, this);
        }

        ~ControlSlider() override
        {
            vts.removeParameterListener (directControlTag, this);
        }

        void parameterChanged (const String& paramID, float newValue) override
        {
            if (paramID != directControlTag)
                return;

            updateSliderVisibility (newValue == 1.0f);
        }

        void colourChanged() override
        {
            for (auto* s : { &freqModSlider, &sensitivitySlider })
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

        void updateSliderVisibility (bool directControlOn)
        {
            sensitivitySlider.setVisible (! directControlOn);
            freqModSlider.setVisible (directControlOn);

            setName (vts.getParameter (directControlOn ? freqModTag : senseTag)->name);
        }

        void visibilityChanged() override
        {
            updateSliderVisibility (vts.getRawParameterValue (directControlTag)->load() == 1.0f);
        }

        void resized() override
        {
            for (auto* s : { &freqModSlider, &sensitivitySlider })
            {
                s->setSliderStyle (getSliderStyle());
                s->setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());
            }

            sensitivitySlider.setBounds (getLocalBounds());
            freqModSlider.setBounds (getLocalBounds());
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

        AudioProcessorValueTreeState& vts;
        Slider freqModSlider, sensitivitySlider;
        std::unique_ptr<SliderAttachment> freqModAttach, senseAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlSlider)
    };

    customComps.add (std::make_unique<ControlSlider> (vts));
}
