#include "Panner.h"
#include "../BufferHelpers.h"
#include "gui/utils/ModulatableSlider.h"
#include "processors/ParameterHelpers.h"

namespace PannerTags
{
const String mainPanTag = "main_pan";
const String leftPanTag = "left_pan";
const String rightPanTag = "right_pan";
const String stereoWidthTag = "stereo_width";
const String modDepthTag = "mod_depth";
const String modRateHzTag = "mod_rate";
const String panModeTag = "pan_mode";
const String stereoModeTag = "stereo_mode";
} // namespace PannerTags

Panner::Panner (UndoManager* um) : BaseProcessor (
                                       "Panner",
                                       createParameterLayout(),
                                       InputPort {},
                                       OutputPort {},
                                       um,
                                       [] (InputPort port)
                                       {
                                           if (port == InputPort::ModulationInput)
                                               return PortType::modulation;
                                           return PortType::audio;
                                       },
                                       [] (OutputPort port)
                                       {
                                           if (port == OutputPort::ModulationOutput)
                                               return PortType::modulation;
                                           return PortType::audio;
                                       })
{
    using namespace ParameterHelpers;
    loadParameterPointer (mainPan, vts, PannerTags::mainPanTag);
    loadParameterPointer (leftPan, vts, PannerTags::leftPanTag);
    loadParameterPointer (rightPan, vts, PannerTags::rightPanTag);
    loadParameterPointer (stereoWidth, vts, PannerTags::stereoWidthTag);
    loadParameterPointer (modDepth, vts, PannerTags::modDepthTag);
    loadParameterPointer (modRateHz, vts, PannerTags::modRateHzTag);
    panMode = vts.getRawParameterValue (PannerTags::panModeTag);
    stereoMode = vts.getRawParameterValue (PannerTags::stereoModeTag);

    uiOptions.backgroundColour = Colours::grey.brighter (0.25f);
    uiOptions.powerColour = Colours::red.brighter (0.1f);
    uiOptions.paramIDsToSkip = { PannerTags::mainPanTag, PannerTags::leftPanTag };
    uiOptions.info.description = "Panning effect with mode and modulation options.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    addPopupMenuParameter (PannerTags::panModeTag);
    addPopupMenuParameter (PannerTags::stereoModeTag);
    disableWhenInputConnected ({ PannerTags::modRateHzTag }, ModulationInput);
}

ParamLayout Panner::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createBipolarPercentParameter (params, PannerTags::mainPanTag, "Pan");
    createBipolarPercentParameter (params, PannerTags::leftPanTag, "Left Pan", -1.0f);
    createBipolarPercentParameter (params, PannerTags::rightPanTag, "Right Pan", 1.0f);
    createPercentParameter (params, PannerTags::stereoWidthTag, "Width", 1.0f);

    createPercentParameter (params, PannerTags::modDepthTag, "Depth", 0.0f);
    createFreqParameter (params, PannerTags::modRateHzTag, "Rate", 0.5f, 10.0f, 2.0f, 1.0f);

    emplace_param<AudioParameterChoice> (params, PannerTags::panModeTag, "Pan Mode", StringArray { "Linear", "Constant Gain", "Constant Power" }, 1);
    emplace_param<AudioParameterChoice> (params, PannerTags::stereoModeTag, "Stereo Mode", StringArray { "Stereo", "Dual" }, 0);

    return { params.begin(), params.end() };
}

void Panner::prepare (double sampleRate, int samplesPerBlock)
{
    for (auto& panner : panners)
        panner.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });

    tempStereoBuffer.setSize (2, samplesPerBlock);

    const auto monoSpec = dsp::ProcessSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    modulator.prepare (monoSpec);
    modulationGain.prepare (monoSpec);
    modulationGain.setRampDurationSeconds (0.02);
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

void Panner::generateModulationSignal (chowdsp::BufferView<float> modulationBuffer)
{
    modulationBuffer.clear();
    auto&& modBlock = modulationBuffer.toAudioBlock();
    auto&& modContext = dsp::ProcessContextReplacing<float> { modBlock };

    if (inputsConnected.contains (ModulationInput))
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        chowdsp::BufferMath::sumToMono (modInputBuffer, modulationBuffer);
    }
    else
    {
        modulator.setFrequency (*modRateHz);
        modulator.process (modContext);
    }

    modulationGain.setGainLinear (*modDepth);
    modulationGain.process (modContext);

    isModulationOn = chowdsp::BufferMath::getMagnitude (modulationBuffer) > 0.001f;
}

void Panner::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    auto stereoBuffer = arena->alloc_buffer (2, numSamples);
    auto modulationBuffer = arena->alloc_buffer (1, numSamples);
    const auto frame = arena->create_frame();

    setPanMode();
    generateModulationSignal (modulationBuffer);

    stereoBuffer.clear();

    if (inputsConnected.contains (AudioInput))
    {
        const auto& inputBuffer = getInputBuffer (AudioInput);
        const auto numChannels = inputBuffer.getNumChannels();

        if (numChannels == 1)
            processMonoInput (inputBuffer, modulationBuffer, stereoBuffer);
        else
            processStereoInput (inputBuffer, modulationBuffer, stereoBuffer);
    }

    outputBuffers.getReference (AudioOutput) = stereoBuffer;
    outputBuffers.getReference (ModulationOutput) = modulationBuffer;
}

void Panner::processSingleChannelPan (chowdsp::Panner<float>& panner,
                                      const chowdsp::BufferView<const float>& inBuffer,
                                      const chowdsp::BufferView<const float>& modulationBuffer,
                                      const chowdsp::BufferView<float>& outBuffer,
                                      float basePanValue,
                                      int inBufferChannel,
                                      float modMultiply) const
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
        auto&& inBlock = inBuffer.toAudioBlock().getSingleChannelBlock ((size_t) inBufferChannel);
        auto&& outBlock = outBuffer.toAudioBlock();
        panner.process (dsp::ProcessContextNonReplacing<float> { inBlock, outBlock });
    }
}

void Panner::processMonoInput (const chowdsp::BufferView<const float>& buffer,
                               const chowdsp::BufferView<const float>& modBuffer,
                               const chowdsp::BufferView<float>& stereoBuffer)
{
    isStereoInput = false;
    processSingleChannelPan (panners[0], buffer, modBuffer, stereoBuffer, *mainPan);
}

void Panner::processStereoInput (const chowdsp::BufferView<const float>& buffer,
                                 const chowdsp::BufferView<const float>& modBuffer,
                                 const chowdsp::BufferView<float>& stereoBuffer)
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

    processSingleChannelPan (panners[0], buffer, modBuffer, stereoBuffer, baseLeftPan);
    processSingleChannelPan (panners[1], buffer, modBuffer, tempStereoBuffer, baseRightPan, 1, -1.0f);

    chowdsp::BufferMath::addBufferData (tempStereoBuffer, stereoBuffer);
}

void Panner::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    auto stereoBuffer = arena->alloc_buffer (2, numSamples);
    auto modulationBuffer = arena->alloc_buffer (1, numSamples);
    const auto frame = arena->create_frame();

    modulationBuffer.clear();
    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        chowdsp::BufferMath::sumToMono (modInputBuffer, modulationBuffer);
    }

    stereoBuffer.clear();
    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        for (int ch = 0; ch < 2; ++ch)
            chowdsp::BufferMath::addBufferChannels (audioInBuffer, stereoBuffer, ch % numChannels, ch);
    }

    outputBuffers.getReference (AudioOutput) = stereoBuffer;
    outputBuffers.getReference (ModulationOutput) = modulationBuffer;
}

bool Panner::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;

    /** Main pan or left pan */
    class PanSlider1 : public Slider
    {
    public:
        PanSlider1 (AudioProcessorValueTreeState& vtState, std::atomic_bool& isStereo, chowdsp::HostContextProvider& hcp)
            : vts (vtState),
              mainPanSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, PannerTags::mainPanTag), hcp),
              leftPanSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, PannerTags::leftPanTag), hcp),
              mainPanAttach (vts, PannerTags::mainPanTag, mainPanSlider),
              leftPanAttach (vts, PannerTags::leftPanTag, leftPanSlider),
              isStereoInput (isStereo),
              stereoAttach (
                  *vts.getParameter (PannerTags::stereoModeTag),
                  [this] (float newValue)
                  { updateSliderVisibility (newValue == 1.0f); },
                  vts.undoManager)
        {
            for (auto* s : { &mainPanSlider, &leftPanSlider })
                addChildComponent (s);

            hcp.registerParameterComponent (mainPanSlider, mainPanSlider.getParameter());
            hcp.registerParameterComponent (leftPanSlider, leftPanSlider.getParameter());

            Component::setName (PannerTags::mainPanTag + "__" + PannerTags::leftPanTag + "__");
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

            setName (vts.getParameter (dualPanOn ? PannerTags::leftPanTag : PannerTags::mainPanTag)->name);
            if (auto* parent = getParentComponent())
                parent->repaint();
        }

        void visibilityChanged() override
        {
            updateSliderVisibility (vts.getRawParameterValue (PannerTags::stereoModeTag)->load() == 1.0f);
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
        PanSlider2 (AudioProcessorValueTreeState& vtState, std::atomic_bool& isStereo, chowdsp::HostContextProvider& hcp)
            : vts (vtState),
              widthSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, PannerTags::stereoWidthTag), hcp),
              rightPanSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, PannerTags::rightPanTag), hcp),
              widthAttach (vts, PannerTags::stereoWidthTag, widthSlider),
              rightPanAttach (vts, PannerTags::rightPanTag, rightPanSlider),
              isStereoInput (isStereo),
              stereoAttach (
                  *vts.getParameter (PannerTags::stereoModeTag),
                  [this] (float newValue)
                  { updateSliderVisibility (newValue == 1.0f); },
                  vts.undoManager)
        {
            for (auto* s : { &widthSlider, &rightPanSlider })
                addChildComponent (s);

            hcp.registerParameterComponent (widthSlider, widthSlider.getParameter());
            hcp.registerParameterComponent (rightPanSlider, rightPanSlider.getParameter());

            Component::setName (PannerTags::stereoWidthTag + "__" + PannerTags::rightPanTag + "__");

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

            setName (vts.getParameter (dualPanOn ? PannerTags::rightPanTag : PannerTags::stereoWidthTag)->name);
            if (auto* parent = getParentComponent())
                parent->repaint();
        }

        void visibilityChanged() override
        {
            updateSliderVisibility (vts.getRawParameterValue (PannerTags::stereoModeTag)->load() == 1.0f);
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
