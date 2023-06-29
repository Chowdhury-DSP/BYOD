#include "CryBaby.h"
#include "CryBabyNDK.h"
#include "gui/utils/ModulatableSlider.h"
#include "processors/BufferHelpers.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace
{
const String controlFreqTag = "control_freq";
const String depthTag = "depth";
const String attackTag = "attack";
const String releaseTag = "release";
const String directControlTag = "direct_control";

// this module needs some extra oversampling to help the Newton-Raphson solver converge
constexpr int oversampleRatio = 2;
} // namespace

CryBaby::CryBaby (UndoManager* um)
    : BaseProcessor (
        "Crying Child",
        createParameterLayout(),
        InputPort {},
        OutputPort {},
        um,
        [] (InputPort port)
        {
            if (port == InputPort::LevelInput)
                return PortType::level;
            return PortType::audio;
        },
        [] (OutputPort port)
        {
            if (port == OutputPort::LevelOutput)
                return PortType::level;
            return PortType::audio;
        })
{
    using namespace ParameterHelpers;
    loadParameterPointer (controlFreqParam, vts, controlFreqTag);
    loadParameterPointer (attackParam, vts, attackTag);
    loadParameterPointer (releaseParam, vts, releaseTag);
    loadParameterPointer (directControlParam, vts, directControlTag);

    depthSmooth.setParameterHandle (getParameterPointer<chowdsp::PercentParameter*> (vts, depthTag));
    depthSmooth.setRampLength (0.025);

    alphaSmooth.setRampLength (0.01);
    alphaSmooth.mappingFunction = [] (float x)
    { return juce::jmap (std::pow (x, 0.5f), 0.1f, 0.99f); };

    uiOptions.backgroundColour = Colours::whitesmoke.darker (0.1f);
    uiOptions.powerColour = Colours::red.darker (0.2f);
    uiOptions.info.description = "\"Wah\" effect based on the Dunlop Cry Baby pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    addPopupMenuParameter (directControlTag);
    disableWhenInputConnected ({ attackTag, releaseTag }, LevelInput);
}

CryBaby::~CryBaby() = default;

ParamLayout CryBaby::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, controlFreqTag, "Freq", 0.5f);
    createPercentParameter (params, depthTag, "Depth", 0.5f);
    createTimeMsParameter (params, attackTag, "Attack", createNormalisableRange (0.1f, 20.0f, 2.0f), 1.0f);
    createTimeMsParameter (params, releaseTag, "Release", createNormalisableRange (1.0f, 200.0f, 20.0f), 25.0f);
    emplace_param<chowdsp::BoolParameter> (params, directControlTag, "Direct Control", false);
    return { params.begin(), params.end() };
}

void CryBaby::prepare (double sampleRate, int samplesPerBlock)
{
    alphaSmooth.prepare (sampleRate, samplesPerBlock);
    depthSmooth.prepare (sampleRate, samplesPerBlock);

    dcBlocker.prepare (2);
    dcBlocker.calcCoefs (30.0f, (float) sampleRate);

    levelDetector.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 });

    audioOutBuffer.setSize (2, samplesPerBlock);
    levelOutBuffer.setSize (1, samplesPerBlock);

    needsOversampling = sampleRate < 88'200.0f;
    if (needsOversampling)
    {
        upsampler.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 }, oversampleRatio);
        downsampler.prepare ({ oversampleRatio * sampleRate, oversampleRatio * (uint32_t) samplesPerBlock, 2 }, oversampleRatio);
    }

    ndk_model = std::make_unique<CryBabyNDK>();
    ndk_model->reset ((needsOversampling ? oversampleRatio : 1.0) * sampleRate);
    const auto alpha = (double) alphaSmooth.getCurrentValue();
    ndk_model->update_pots ({ (1.0 - alpha) * CryBabyNDK::VR1, alpha * CryBabyNDK::VR1 });

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    float level = 100.0f;
    while (level > 1.0e-4f)
    {
        buffer.clear();
        processBlockNDK (buffer);
        level = buffer.getMagnitude (0, samplesPerBlock);
    }
}

void CryBaby::processBlockNDK (const chowdsp::BufferView<float>& block, int smootherDivide)
{
    depthSmooth.process (block.getNumSamples());
    const auto depthSmoothData = depthSmooth.getSmoothedBuffer();
    const auto levelInputData = levelOutBuffer.getReadPointer (0);

    for (auto [ch, n, data] : chowdsp::buffer_iters::sub_blocks<32, true> (block))
    {
        if (ch == 0)
        {
            auto targetFreqControl = controlFreqParam->getCurrentValue();
            if (! directControlParam->get())
                targetFreqControl += 0.98f * depthSmoothData[n / smootherDivide] * levelInputData[n / smootherDivide];
            alphaSmooth.process (jlimit (0.0f, 1.0f, targetFreqControl), (int) data.size() / smootherDivide);

            const auto alpha = (double) alphaSmooth.getCurrentValue();
            ndk_model->update_pots ({ (1.0 - alpha) * CryBabyNDK::VR1, alpha * CryBabyNDK::VR1 });
        }

        ndk_model->process (data, ch);
    }
}

void CryBaby::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    levelOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (LevelInput))
    {
        BufferHelpers::collapseToMonoBuffer (getInputBuffer (LevelInput), levelOutBuffer);
    }
    else
    {
        if (inputsConnected.contains (AudioInput))
        {
            levelDetector.setParameters (attackParam->getCurrentValue(), releaseParam->getCurrentValue());
            levelDetector.processBlock (getInputBuffer (AudioInput), levelOutBuffer);
        }
        else
        {
            levelOutBuffer.clear();
        }
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        audioOutBuffer.makeCopyOf (audioInBuffer, true);

        dcBlocker.processBlock (buffer);

        if (needsOversampling)
        {
            const auto osBufferView = upsampler.process (audioOutBuffer);
            processBlockNDK (osBufferView, oversampleRatio);
            downsampler.process (osBufferView, audioOutBuffer);
        }
        else
        {
            processBlockNDK (audioOutBuffer);
        }
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (LevelOutput) = &levelOutBuffer;
}

void CryBaby::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    levelOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (LevelInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& levelInputBuffer = getInputBuffer (LevelInput);
        BufferHelpers::collapseToMonoBuffer (levelInputBuffer, levelOutBuffer);
    }
    else
    {
        levelOutBuffer.clear();
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);
        for (int ch = 0; ch < numChannels; ++ch)
            audioOutBuffer.copyFrom (ch, 0, audioInBuffer, ch % numChannels, 0, numSamples);
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (LevelOutput) = &levelOutBuffer;
}

bool CryBaby::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    class CustomSlider : public Slider
    {
    public:
        CustomSlider (AudioProcessorValueTreeState& vtState, const String& tag, chowdsp::HostContextProvider& hcp)
            : vts (vtState),
              internalSlider (*chowdsp::ParamUtils::getParameterPointer<chowdsp::FloatParameter*> (vts, tag), hcp),
              attach (vts, tag, internalSlider),
              directControlAttach (*chowdsp::ParamUtils::getParameterPointer<chowdsp::BoolParameter*> (vts, directControlTag),
                                   [this] (float newValue)
                                   {
                                       internalSlider.setEnabled (newValue < 0.5f);
                                   })
        {
            addAndMakeVisible (internalSlider);
            directControlAttach.sendInitialUpdate();

            hcp.registerParameterComponent (internalSlider, internalSlider.getParameter());

            Slider::setName (tag + "__");
        }

        void colourChanged() override
        {
            for (auto colourID : { Slider::textBoxOutlineColourId,
                                   Slider::textBoxTextColourId,
                                   Slider::textBoxBackgroundColourId,
                                   Slider::textBoxHighlightColourId,
                                   Slider::thumbColourId })
            {
                internalSlider.setColour (colourID, findColour (colourID, false));
            }
        }

        void resized() override
        {
            Slider::setName (internalSlider.getParameter().getName (25));
            internalSlider.setSliderStyle (getSliderStyle());
            internalSlider.setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());
            internalSlider.setBounds (getLocalBounds());
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

        AudioProcessorValueTreeState& vts;
        ModulatableSlider internalSlider;
        SliderAttachment attach;
        ParameterAttachment directControlAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomSlider)
    };

    customComps.add (std::make_unique<CustomSlider> (vts, attackTag, hcp));
    customComps.add (std::make_unique<CustomSlider> (vts, releaseTag, hcp));
    customComps.add (std::make_unique<CustomSlider> (vts, depthTag, hcp));

    return true;
}
