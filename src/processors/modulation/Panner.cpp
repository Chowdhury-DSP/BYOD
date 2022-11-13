#include "Panner.h"
#include "../BufferHelpers.h"
#include "gui/utils/ModulatableSlider.h"
#include "processors/ParameterHelpers.h"

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

Panner::Panner (UndoManager* um) : BaseProcessor ("Panner",
                                                  createParameterLayout(),
                                                  um,
                                                  magic_enum::enum_count<InputPort>(),
                                                  magic_enum::enum_count<OutputPort>())
{
    using namespace ParameterHelpers;
    loadParameterPointer (mainPan, vts, mainPanTag);
    loadParameterPointer (leftPan, vts, leftPanTag);
    loadParameterPointer (rightPan, vts, rightPanTag);
    loadParameterPointer (stereoWidth, vts, stereoWidthTag);
    loadParameterPointer (modDepth, vts, modDepthTag);
    loadParameterPointer (modRateHz, vts, modRateHzTag);
    panMode = vts.getRawParameterValue (panModeTag);
    stereoMode = vts.getRawParameterValue (stereoModeTag);

    uiOptions.backgroundColour = Colours::grey.brighter (0.25f);
    uiOptions.powerColour = Colours::red.brighter (0.1f);
    uiOptions.paramIDsToSkip = { mainPanTag, leftPanTag };
    uiOptions.info.description = "Panning effect with mode and modulation options.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    addPopupMenuParameter (panModeTag);
    addPopupMenuParameter (stereoModeTag);
    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ modRateHzTag }, ModulationInput);
}

ParamLayout Panner::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createBipolarPercentParameter (params, mainPanTag, "Pan");
    createBipolarPercentParameter (params, leftPanTag, "Left Pan", -1.0f);
    createBipolarPercentParameter (params, rightPanTag, "Right Pan", 1.0f);
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

    if (inputsConnected.contains (ModulationInput))
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modulationBuffer);
    }
    else
    {
        modulator.setFrequency (*modRateHz);
        modulator.process (modContext);
    }

    modulationGain.setGainLinear (*modDepth);
    modulationGain.process (modContext);

    isModulationOn = modulationBuffer.getMagnitude (0, numSamples) > 0.001f;
}

void Panner::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    setPanMode();
    generateModulationSignal (numSamples);

    stereoBuffer.setSize (2, numSamples, false, false, true);
    stereoBuffer.clear();

    if (inputsConnected.contains (AudioInput))
    {
        const auto& inputBuffer = getInputBuffer (AudioInput);
        const auto numChannels = inputBuffer.getNumChannels();

        if (numChannels == 1)
            processMonoInput (inputBuffer);
        else
            processStereoInput (inputBuffer);
    }

    outputBuffers.getReference (AudioOutput) = &stereoBuffer;
    outputBuffers.getReference (ModulationOutput) = &modulationBuffer;
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

    auto baseLeftPan = leftPan->getCurrentValue();
    auto baseRightPan = rightPan->getCurrentValue();
    if (*stereoMode == 0.0f) // stereo pan
    {
        const auto currentMainPan = mainPan->getCurrentValue();
        const auto widthMult = stereoWidth->getCurrentValue();
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
    const auto numSamples = buffer.getNumSamples();

    modulationBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modulationBuffer);
    }
    else
    {
        modulationBuffer.clear();
    }

    stereoBuffer.setSize (2, numSamples, false, false, true);
    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        for (int ch = 0; ch < 2; ++ch)
            stereoBuffer.copyFrom (ch, 0, audioInBuffer, ch % numChannels, 0, numSamples);
    }
    else
    {
        stereoBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &stereoBuffer;
    outputBuffers.getReference (ModulationOutput) = &modulationBuffer;
}

bool Panner::getCustomComponents (OwnedArray<Component>& customComps, HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;

    /** Main pan or left pan */
    class PanSlider1 : public Slider
    {
    public:
        PanSlider1 (AudioProcessorValueTreeState& vtState, std::atomic_bool& isStereo, HostContextProvider& hcp)
            : vts (vtState),
              mainPanSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, mainPanTag), hcp),
              leftPanSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, leftPanTag), hcp),
              mainPanAttach (vts, mainPanTag, mainPanSlider),
              leftPanAttach (vts, leftPanTag, leftPanSlider),
              isStereoInput (isStereo),
              stereoAttach (
                  *vts.getParameter (stereoModeTag),
                  [this] (float newValue)
                  { updateSliderVisibility (newValue == 1.0f); },
                  vts.undoManager)
        {
            for (auto* s : { &mainPanSlider, &leftPanSlider })
                addChildComponent (s);

            hcp.registerParameterComponent (mainPanSlider, mainPanSlider.getParameter());
            hcp.registerParameterComponent (leftPanSlider, leftPanSlider.getParameter());

            this->setName (mainPanTag + "__" + leftPanTag + "__");
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
            if (auto* parent = getParentComponent())
                parent->repaint();
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
        ModulatableSlider mainPanSlider, leftPanSlider;
        SliderAttachment mainPanAttach, leftPanAttach;
        std::atomic_bool& isStereoInput;
        ParameterAttachment stereoAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanSlider1)
    };

    /** Width or right pan */
    class PanSlider2 : public Slider,
                       private Timer
    {
    public:
        PanSlider2 (AudioProcessorValueTreeState& vtState, std::atomic_bool& isStereo, HostContextProvider& hcp)
            : vts (vtState),
              widthSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, stereoWidthTag), hcp),
              rightPanSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, rightPanTag), hcp),
              widthAttach (vts, stereoWidthTag, widthSlider),
              rightPanAttach (vts, rightPanTag, rightPanSlider),
              isStereoInput (isStereo),
              stereoAttach (
                  *vts.getParameter (stereoModeTag),
                  [this] (float newValue)
                  { updateSliderVisibility (newValue == 1.0f); },
                  vts.undoManager)
        {
            for (auto* s : { &widthSlider, &rightPanSlider })
                addChildComponent (s);

            hcp.registerParameterComponent (widthSlider, widthSlider.getParameter());
            hcp.registerParameterComponent (rightPanSlider, rightPanSlider.getParameter());

            this->setName (stereoWidthTag + "__" + rightPanTag + "__");

            startTimerHz (10);
        }

        void timerCallback() override
        {
            setEnabled (isStereoInput);
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
            if (auto* parent = getParentComponent())
                parent->repaint();
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
        ModulatableSlider widthSlider, rightPanSlider;
        SliderAttachment widthAttach, rightPanAttach;
        std::atomic_bool& isStereoInput;
        ParameterAttachment stereoAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanSlider2)
    };

    customComps.add (std::make_unique<PanSlider1> (vts, isStereoInput, hcp));
    customComps.add (std::make_unique<PanSlider2> (vts, isStereoInput, hcp));

    return true;
}
