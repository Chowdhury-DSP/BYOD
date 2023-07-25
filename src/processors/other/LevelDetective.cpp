#include "LevelDetective.h"
#include "../ParameterHelpers.h"
#include "gui/utils/ModulatableSlider.h"

using namespace chowdsp::compressor;

namespace
{
const auto powerColour = Colours::gold.darker (0.1f);
const auto backgroundColour = Colours::teal.darker (0.1f);
const String attackTag = "attack";
const String releaseTag = "release";
} // namespace

LevelDetective::LevelDetective (UndoManager* um) : BaseProcessor (
    "Level Detective",
    createParameterLayout(),
    InputPort {},
    OutputPort {},
    um,
    [] (InputPort)
    {
        return PortType::audio;
    },
    [] (OutputPort)
    {
        return PortType::level;
    })
{
    using namespace ParameterHelpers;
    loadParameterPointer (attackMsParam, vts, attackTag);
    loadParameterPointer (releaseMsParam, vts, releaseTag);
    uiOptions.backgroundColour = backgroundColour;
    uiOptions.powerColour = powerColour;
    uiOptions.info.description = "A simple envelope follower";
    uiOptions.info.authors = StringArray { "Rachel Locke" };
}

ParamLayout LevelDetective::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createTimeMsParameter (params, attackTag, "Attack", createNormalisableRange (1.0f, 100.0f, 10.0f), 10.0f);
    createTimeMsParameter (params, releaseTag, "Release", createNormalisableRange (10.0f, 1000.0f, 100.0f), 400.0f);

    return { params.begin(), params.end() };
}

void LevelDetective::prepare (double sampleRate, int samplesPerBlock)
{
    levelOutBuffer.setSize (1, samplesPerBlock);
    level.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });

    levelVisualizer.setBufferSize (int (levelVisualizer.secondsToVisualize * sampleRate / (double) samplesPerBlock));
    levelVisualizer.setSamplesPerBlock (samplesPerBlock);
}

void LevelDetective::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    levelOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (AudioInput))
    {
        nonstd::span<const float> audioChannelData = { buffer.getReadPointer (0), (size_t) numSamples };
        levelVisualizer.pushChannel (0, audioChannelData);
        level.setParameters (*attackMsParam, *releaseMsParam);
        level.processBlock (buffer, levelOutBuffer);

        nonstd::span<const float> levelChannelData = { levelOutBuffer.getReadPointer (0), (size_t) numSamples };
        levelVisualizer.pushChannel (1, levelChannelData);
    }
    else
    {
        levelOutBuffer.clear();
    }

    outputBuffers.getReference (LevelOutput) = &levelOutBuffer;
}

void LevelDetective::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    levelOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (AudioInput))
    {
        levelOutBuffer.clear();
        nonstd::span<const float> audioChannelData = { buffer.getReadPointer (0), (size_t) numSamples };
        nonstd::span<const float> levelChannelData = { levelOutBuffer.getReadPointer (0), (size_t) numSamples };
        levelVisualizer.pushChannel (0, audioChannelData);
        levelVisualizer.pushChannel (1, levelChannelData);
        outputBuffers.getReference (LevelOutput) = &levelOutBuffer;
    }
}

bool LevelDetective::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;
    class LevelDetectiveEditor : public juce::Component
    {
    public:
        explicit LevelDetectiveEditor (LevelDetectorVisualizer& viz, AudioProcessorValueTreeState& vtState, chowdsp::HostContextProvider& hcp)
            : visualiser (viz),
              vts (vtState),
              attackSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, attackTag), hcp),
              releaseSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, releaseTag), hcp),
              attackAttach (vts, attackTag, attackSlider),
              releaseAttach (vts, releaseTag, releaseSlider)
        {
            attackLabel.setText ("Attack", juce::dontSendNotification);
            releaseLabel.setText ("Release", juce::dontSendNotification);
            attackLabel.setJustificationType (Justification::centred);
            releaseLabel.setJustificationType (Justification::centred);

            addAndMakeVisible (attackLabel);
            addAndMakeVisible (releaseLabel);

            for (auto* s : { &attackSlider, &releaseSlider })
            {
                s->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
                s->setTextBoxStyle (Slider::TextBoxBelow, false, 80, 20);
                s->setColour (Slider::textBoxHighlightColourId, powerColour.withAlpha (0.55f));
                s->setColour (Slider::thumbColourId, powerColour);
                addAndMakeVisible (s);
            }

            visualiser.backgroundColour = backgroundColour.darker (0.4f);
            visualiser.audioColour = powerColour;

            hcp.registerParameterComponent (attackSlider, attackSlider.getParameter());
            hcp.registerParameterComponent (releaseSlider, releaseSlider.getParameter());

            this->setName (attackTag + "__" + releaseTag + "__");
            addAndMakeVisible (visualiser);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            visualiser.setBounds (bounds.removeFromTop (proportionOfHeight (0.33f)));
            bounds.removeFromTop (proportionOfHeight (0.03f));
            auto labelRect = bounds.removeFromTop (proportionOfHeight (0.115f));
            attackLabel.setBounds (labelRect.removeFromLeft (proportionOfWidth (0.5f)));
            attackLabel.setFont (Font ((float) attackLabel.getHeight() - 2.0f).boldened());
            releaseLabel.setBounds (labelRect);
            releaseLabel.setFont (Font ((float) releaseLabel.getHeight() - 2.0f).boldened());
            attackSlider.setBounds (bounds.removeFromLeft (proportionOfWidth (0.5f)));
            releaseSlider.setBounds (bounds);
            for (auto* s : { &attackSlider, &releaseSlider })
                s->setTextBoxStyle (Slider::TextBoxBelow, false, proportionOfWidth (0.4f), proportionOfHeight (0.137f));
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
        LevelDetectorVisualizer& visualiser;
        AudioProcessorValueTreeState& vts;
        ModulatableSlider attackSlider, releaseSlider;
        SliderAttachment attackAttach, releaseAttach;
        Label attackLabel, releaseLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelDetectiveEditor)
    };

    customComps.add (std::make_unique<LevelDetectiveEditor> (levelVisualizer, vts, hcp));
    return false;
}