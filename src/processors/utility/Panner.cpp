#include "Panner.h"
#include "../ParameterHelpers.h"

namespace
{
const String mainPanTag = "main_pan";
const String leftPanTag = "left_pan";
const String rightPanTag = "right_pan";
const String stereoWidthTag = "stereo_width";
const String modDepthTag = "mod_depth";
const String modRateHzTag = "mod_rate";
const String panModeTag = "pan_mode";
const String stereoModeTag = "stereo_mode";
} // namespace

Panner::Panner (UndoManager* um) : BaseProcessor ("Panner", createParameterLayout(), um)
{
    mainPan = vts.getRawParameterValue (mainPanTag);
    leftPan = vts.getRawParameterValue (leftPanTag);
    rightPan = vts.getRawParameterValue (rightPanTag);
    stereoWidth = vts.getRawParameterValue (stereoWidthTag);
    modDepth = vts.getRawParameterValue (modDepthTag);
    modRateHz = vts.getRawParameterValue (modRateHzTag);
    panMode = vts.getRawParameterValue (panModeTag);
    stereoMode = vts.getRawParameterValue (stereoModeTag);

    uiOptions.backgroundColour = Colours::grey.brighter (0.25f);
    uiOptions.powerColour = Colours::red.brighter (0.1f);
    uiOptions.paramIDsToSkip = { mainPanTag, leftPanTag };
    uiOptions.info.description = "Panning effect with mode and modulation options.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    addPopupMenuParameter (panModeTag);
    addPopupMenuParameter (stereoModeTag);
}

ParamLayout Panner::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createBidirectionalPercentParameter (params, mainPanTag, "Pan");
    createBidirectionalPercentParameter (params, leftPanTag, "Left Pan", -1.0f);
    createBidirectionalPercentParameter (params, rightPanTag, "Right Pan", 1.0f);
    createPercentParameter (params, stereoWidthTag, "Width", 1.0f);

    createPercentParameter (params, modDepthTag, "Depth", 0.0f);
    createFreqParameter (params, modRateHzTag, "Rate", 0.5f, 10.0f, 2.0f, 1.0f);

    emplace_param<AudioParameterChoice> (params, panModeTag, "Pan Mode", StringArray { "Linear", "Constant Gain", "Constant Power" }, 1);
    emplace_param<AudioParameterChoice> (params, stereoModeTag, "Stereo Mode", StringArray { "Stereo", "Dual" }, 0);

    return { params.begin(), params.end() };
}

void Panner::prepare (double sampleRate, int samplesPerBlock)
{
    for (auto& panner : panners)
        panner.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });

    stereoBuffer.setSize (2, samplesPerBlock);
    tempStereoBuffer.setSize (2, samplesPerBlock);

    const auto monoSpec = dsp::ProcessSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    modulator.prepare (monoSpec);
    modulationGain.prepare (monoSpec);
    modulationGain.setRampDurationSeconds (0.02);
    modulationBuffer.setSize (1, samplesPerBlock);
}

void Panner::setPanMode()
{
    using PanRule = chowdsp::Panner<float>::Rule;
    auto getPanRule = [] (int panModeInt)
    {
        switch (panModeInt)
        {
            case 0:
                return PanRule::linear;
            case 1:
                return PanRule::sin6dB;
            case 2:
                return PanRule::sin3dB;
            default:
                jassertfalse; // unknown pan rule choice!
                return PanRule::linear;
        }
    };

    auto panRule = getPanRule ((int) *panMode);
    for (auto& panner : panners)
        panner.setRule (panRule);
}

void Panner::generateModulationSignal (int numSamples)
{
    modulationBuffer.setSize (1, numSamples, false, false, true);
    modulationBuffer.clear();
    auto&& modBlock = dsp::AudioBlock<float> { modulationBuffer };
    auto&& modContext = dsp::ProcessContextReplacing<float> { modBlock };

    modulator.setFrequency (*modRateHz);
    modulator.process (modContext);

    modulationGain.setGainLinear (*modDepth);
    modulationGain.process (modContext);

    isModulationOn = modulationBuffer.getMagnitude (0, numSamples) > 0.001f;
}

void Panner::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    setPanMode();
    generateModulationSignal (numSamples);

    stereoBuffer.setSize (2, numSamples, false, false, true);
    stereoBuffer.clear();

    if (numChannels == 1)
        processMonoInput (buffer);
    else
        processStereoInput (buffer);

    outputBuffers.getReference (0) = &stereoBuffer;
}

void Panner::processSingleChannelPan (chowdsp::Panner<float>& panner, const AudioBuffer<float>& inBuffer, AudioBuffer<float>& outBuffer, float basePanValue, int inBufferChannel, float modMultiply)
{
    if (isModulationOn)
    {
        const auto numSamples = inBuffer.getNumSamples();
        const auto* inData = inBuffer.getReadPointer (inBufferChannel);
        const auto* modData = modulationBuffer.getReadPointer (0);
        auto* leftData = outBuffer.getWritePointer (0);
        auto* rightData = outBuffer.getWritePointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            panner.setPan (jlimit (-1.0f, 1.0f, basePanValue + modData[n] * modMultiply));
            std::tie (leftData[n], rightData[n]) = panner.processSample (inData[n]);
        }
    }
    else // no modulation
    {
        panner.setPan (basePanValue);
        auto&& inBlock = dsp::AudioBlock<const float> { inBuffer }.getSingleChannelBlock ((size_t) inBufferChannel);
        auto&& outBlock = dsp::AudioBlock<float> { outBuffer };
        panner.process (dsp::ProcessContextNonReplacing<float> { inBlock, outBlock });
    }
}

void Panner::processMonoInput (const AudioBuffer<float>& buffer)
{
    isStereoInput = false;
    processSingleChannelPan (panners[0], buffer, stereoBuffer, *mainPan);
}

void Panner::processStereoInput (const AudioBuffer<float>& buffer)
{
    isStereoInput = true;

    const auto numSamples = buffer.getNumSamples();
    tempStereoBuffer.setSize (2, numSamples, false, false, true);
    tempStereoBuffer.clear();

    auto baseLeftPan = leftPan->load();
    auto baseRightPan = rightPan->load();
    if (*stereoMode == 0.0f) // stereo pan
    {
        const auto currentMainPan = mainPan->load();
        const auto widthMult = stereoWidth->load();
        baseLeftPan = jmax (currentMainPan * 2.0f - 1.0f, -1.0f) * widthMult;
        baseRightPan = jmin (currentMainPan * 2.0f + 1.0f, 1.0f) * widthMult;
    }

    processSingleChannelPan (panners[0], buffer, stereoBuffer, baseLeftPan);
    processSingleChannelPan (panners[1], buffer, tempStereoBuffer, baseRightPan, 1, -1.0f);

    for (int ch = 0; ch < 2; ++ch)
        stereoBuffer.addFrom (ch, 0, tempStereoBuffer, ch, 0, numSamples);
}

void Panner::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    stereoBuffer.setSize (2, numSamples, false, false, true);

    for (int ch = 0; ch < 2; ++ch)
        stereoBuffer.copyFrom (ch, 0, buffer, ch % numChannels, 0, numSamples);

    outputBuffers.getReference (0) = &stereoBuffer;
}

bool Panner::getCustomComponents (OwnedArray<Component>& customComps)
{
    /** Main pan or left pan */
    class PanSlider1 : public Slider,
                       private AudioProcessorValueTreeState::Listener,
                       private Timer
    {
    public:
        explicit PanSlider1 (AudioProcessorValueTreeState& vtState, std::atomic_bool& isStereo) : vts (vtState),
                                                                                                  isStereoInput (isStereo)
        {
            for (auto* s : { &mainPanSlider, &leftPanSlider })
                addChildComponent (s);

            mainPanAttach = std::make_unique<SliderAttachment> (vts, mainPanTag, mainPanSlider);
            leftPanAttach = std::make_unique<SliderAttachment> (vts, leftPanTag, leftPanSlider);

            setName (mainPanTag + "__" + leftPanTag + "__");

            vts.addParameterListener (stereoModeTag, this);

            startTimerHz (10);
        }

        ~PanSlider1() override
        {
            vts.removeParameterListener (stereoModeTag, this);
        }

        void timerCallback() override
        {
            updateSliderVisibility (vts.getRawParameterValue (stereoModeTag)->load() == 1.0f);
        }

        void parameterChanged (const String& paramID, float newValue) override
        {
            if (paramID != stereoModeTag)
                return;

            updateSliderVisibility (newValue == 1.0f);
        }

        void colourChanged() override
        {
            for (auto* s : { &mainPanSlider, &leftPanSlider })
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

        void updateSliderVisibility (bool dualPanOn)
        {
            if (! isStereoInput)
                dualPanOn = false;

            mainPanSlider.setVisible (! dualPanOn);
            leftPanSlider.setVisible (dualPanOn);

            setName (vts.getParameter (dualPanOn ? leftPanTag : mainPanTag)->name);
        }

        void visibilityChanged() override
        {
            updateSliderVisibility (vts.getRawParameterValue (stereoModeTag)->load() == 1.0f);
        }

        void resized() override
        {
            for (auto* s : { &mainPanSlider, &leftPanSlider })
            {
                s->setSliderStyle (getSliderStyle());
                s->setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());
            }

            mainPanSlider.setBounds (getLocalBounds());
            leftPanSlider.setBounds (getLocalBounds());
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

        AudioProcessorValueTreeState& vts;
        Slider mainPanSlider, leftPanSlider;
        std::unique_ptr<SliderAttachment> mainPanAttach, leftPanAttach;
        std::atomic_bool& isStereoInput;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanSlider1)
    };

    /** Width or right pan */
    class PanSlider2 : public Slider,
                       private AudioProcessorValueTreeState::Listener,
                       private Timer
    {
    public:
        explicit PanSlider2 (AudioProcessorValueTreeState& vtState, std::atomic_bool& isStereo) : vts (vtState),
                                                                                                  isStereoInput (isStereo)
        {
            for (auto* s : { &widthSlider, &rightPanSlider })
                addChildComponent (s);

            widthAttach = std::make_unique<SliderAttachment> (vts, stereoWidthTag, widthSlider);
            rightPanAttach = std::make_unique<SliderAttachment> (vts, rightPanTag, rightPanSlider);

            setName (stereoWidthTag + "__" + rightPanTag + "__");

            vts.addParameterListener (stereoModeTag, this);

            startTimerHz (10);
        }

        ~PanSlider2() override
        {
            vts.removeParameterListener (stereoModeTag, this);
        }

        void timerCallback() override
        {
            setEnabled (isStereoInput);
        }

        void parameterChanged (const String& paramID, float newValue) override
        {
            if (paramID != stereoModeTag)
                return;

            updateSliderVisibility (newValue == 1.0f);
        }

        void colourChanged() override
        {
            for (auto* s : { &widthSlider, &rightPanSlider })
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

        void updateSliderVisibility (bool dualPanOn)
        {
            widthSlider.setVisible (! dualPanOn);
            rightPanSlider.setVisible (dualPanOn);

            setName (vts.getParameter (dualPanOn ? rightPanTag : stereoWidthTag)->name);
        }

        void visibilityChanged() override
        {
            updateSliderVisibility (vts.getRawParameterValue (stereoModeTag)->load() == 1.0f);
        }

        void resized() override
        {
            for (auto* s : { &widthSlider, &rightPanSlider })
            {
                s->setSliderStyle (getSliderStyle());
                s->setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());
            }

            widthSlider.setBounds (getLocalBounds());
            rightPanSlider.setBounds (getLocalBounds());
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

        AudioProcessorValueTreeState& vts;
        Slider widthSlider, rightPanSlider;
        std::unique_ptr<SliderAttachment> widthAttach, rightPanAttach;
        std::atomic_bool& isStereoInput;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanSlider2)
    };

    customComps.add (std::make_unique<PanSlider1> (vts, isStereoInput));
    customComps.add (std::make_unique<PanSlider2> (vts, isStereoInput));

    return true;
}
