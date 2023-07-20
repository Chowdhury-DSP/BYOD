#include "LevelDetective.h"
#include "../ParameterHelpers.h"

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
    //    levelVisualizer = std::make_unique<LevelDetectorVisualizer>();
    using namespace ParameterHelpers;
    //    loadParameterPointer (attackMsParam, vts, "attack");
    //    loadParameterPointer (releaseMsParam, vts, "release");
    uiOptions.backgroundColour = Colours::teal.darker (0.1f);
    uiOptions.powerColour = Colours::gold.darker (0.1f);
    uiOptions.info.description = "A simple envelope follower";
    uiOptions.info.authors = StringArray { "Rachel Locke" };
}

ParamLayout LevelDetective::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    //    createTimeMsParameter (params, "attack", "Attack", createNormalisableRange (1.0f, 100.0f, 10.0f), 10.0f);
    //    createTimeMsParameter (params, "release", "Release", createNormalisableRange (10.0f, 1000.0f, 100.0f), 400.0f);

    return { params.begin(), params.end() };
}

void LevelDetective::prepare (double sampleRate, int samplesPerBlock)
{
    //prepare the buffers
    levelOutBuffer.setSize (1, samplesPerBlock);
    level.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });

    levelVisualizer.setBufferSize (int (levelVisualizer.secondsToVisualize * sampleRate / (double) samplesPerBlock));
    levelVisualizer.setSamplesPerBlock (samplesPerBlock);
}

void LevelDetective::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    levelOutBuffer.setSize (1, numSamples, false, false, true);
    //if audio input connected, extract level from input signal and assign to levelOutBuffer
    if (inputsConnected.contains (AudioInput))
    {
        //create span to fill audio visualiser buffer
        nonstd::span<const float> audioChannelData = { buffer.getReadPointer (0), (size_t) numSamples };
        levelVisualizer.pushChannel (0, audioChannelData);
        //        level.setParameters(*attackMsParam, *releaseMsParam);
        level.processBlock (buffer, levelOutBuffer);

        //create span to fill level visualiser buffer
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
        outputBuffers.getReference (LevelOutput) = &levelOutBuffer;
    }
}

bool LevelDetective::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&)
{
    struct LevelDetectiveEditor : juce::Component
    {
        explicit LevelDetectiveEditor (juce::Component& viz) : visualiser (viz)
        {
            addAndMakeVisible (visualiser);
        }

        void resized() override
        {
            visualiser.setBounds (getLocalBounds());
        }

        juce::Component& visualiser;
    };

    customComps.add (std::make_unique<LevelDetectiveEditor> (levelVisualizer));

    return false;
}