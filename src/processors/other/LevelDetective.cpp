#include "LevelDetective.h"
#include "../ParameterHelpers.h"

LevelDetective::LevelDetective (UndoManager* um) : BaseProcessor (
    "Level Detective",
    createParameterLayout(),
    InputPort {},
    OutputPort {},
    um,
    [] (InputPort port)
    {
        return PortType::audio;
    },
    [] (OutputPort port)
    {
        return PortType::level;
    })
{
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
    //if audio input connected, extract level from input signal and assign to levelOutBuffer
    if (inputsConnected.contains (AudioInput))
    {
        //create span to fill audio visualiser buffer
        nonstd::span<const float> audioChannelData = { buffer.getReadPointer (0), (size_t) buffer.getNumSamples() };
        levelVisualizer.pushChannel (0, audioChannelData);
        //        level.setParameters(*attackMsParam, *releaseMsParam);

        level.processBlock (buffer, levelOutBuffer);

        //create span to fill audio visualiser buffer
        nonstd::span<const float> levelChannelData = { levelOutBuffer.getReadPointer (0), (size_t) levelOutBuffer.getNumSamples() };
        levelVisualizer.pushChannel (1, levelChannelData);
    }
    else
    {
        levelOutBuffer.clear();
    }

    outputBuffers.getReference (LevelOutput) = &levelOutBuffer;
}

bool LevelDetective::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&)
{
    struct LevelDetectiveEditor : juce::Component
    {
        //Parameters here?

        juce::Component* visualiser;
    } levelDetectiveEditor;

    levelDetectiveEditor.visualiser = &levelVisualizer;

    customComps.add (levelDetectiveEditor.visualiser);
    return false;
}