#include "Phaser8.h"
#include "processors/BufferHelpers.h"
#include "processors/ParameterHelpers.h"

namespace
{
const String rateTag = "rate";
const String depthTag = "depth";
const String feedbackTag = "feedback";
const String modulationTag = "modulation";
} // namespace

Phaser8::Phaser8 (UndoManager* um) : BaseProcessor (
    "Phaser8",
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
    loadParameterPointer (rateHzParam, vts, rateTag);

    depthParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, depthTag));
    depthParam.mappingFunction = [] (float x)
    { return 0.45f * x; };

    feedbackParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, feedbackTag));
    feedbackParam.mappingFunction = [] (float x)
    { return 0.95f * x; };

    modSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, modulationTag));
    noModSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, modulationTag));
    noModSmooth.mappingFunction = [] (float x)
    { return 1.0f - x; };

    lfoShaper.initialise ([] (float x)
                          {
                              static constexpr auto skewFactor = gcem::pow (2.0f, -0.25f);
                              return 2.0f * std::pow ((x + 1.0f) * 0.5f, skewFactor) - 1.0f; },
                          -1.0f,
                          1.0f,
                          2048);

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ rateTag }, ModulationInput);

    uiOptions.backgroundColour = Colour { 0xff00a8e9 };
    uiOptions.powerColour = Colour { 0xfff44e44 };
    uiOptions.info.description = "A phaser effect based on a classic \"Compact\" 8-stage phaser pedal. The first output contains the signal after the pedal's feedback and modulation stages. The second output contains the sigal after only the feedback stage.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Phaser8::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createFreqParameter (params, rateTag, "Rate", 0.1f, 20.0f, 1.0f, 1.0f);
    createPercentParameter (params, depthTag, "Depth", 1.0f);
    createPercentParameter (params, feedbackTag, "Feedback", 0.75f);
    createPercentParameter (params, modulationTag, "Modulation", 0.75f);

    return { params.begin(), params.end() };
}

void Phaser8::prepare (double sampleRate, int samplesPerBlock)
{
    depthParam.prepare (sampleRate, samplesPerBlock);
    depthParam.setRampLength (0.05);
    feedbackParam.prepare (sampleRate, samplesPerBlock);
    feedbackParam.setRampLength (0.05);
    modSmooth.prepare (sampleRate, samplesPerBlock);
    modSmooth.setRampLength (0.05);
    noModSmooth.prepare (sampleRate, samplesPerBlock);
    noModSmooth.setRampLength (0.05);

    fbStage.prepare ((float) sampleRate);
    fbStageNoMod.prepare ((float) sampleRate);
    modStages.prepare ((float) sampleRate);

    sineLFO.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 1 });
    modData.resize ((size_t) samplesPerBlock, 0.0f);

    modulatedOutBuffer.setSize (1, samplesPerBlock);
    nonModulatedOutBuffer.setSize (1, samplesPerBlock);
    modOutBuffer.setSize (1, samplesPerBlock);
}

void Phaser8::processModulation (int numSamples)
{
    modOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (ModulationInput))
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
        FloatVectorOperations::clip (modOutBuffer.getWritePointer (0),
                                     modOutBuffer.getReadPointer (0),
                                     -1.0f,
                                     1.0f,
                                     numSamples);
    }
    else
    {
        sineLFO.setFrequency (rateHzParam->getCurrentValue());
        modOutBuffer.clear();
        sineLFO.processBlock (modOutBuffer);

        lfoShaper.process (modOutBuffer.getReadPointer (0),
                           modOutBuffer.getWritePointer (0),
                           numSamples);
    }

    FloatVectorOperations::multiply (modData.data(),
                                     modOutBuffer.getReadPointer (0),
                                     depthParam.getSmoothedBuffer(),
                                     numSamples);
    FloatVectorOperations::add (modData.data(), 0.5f, numSamples);
}

void Phaser8::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    depthParam.process (numSamples);
    feedbackParam.process (numSamples);
    modSmooth.process (numSamples);
    noModSmooth.process (numSamples);
    processModulation (numSamples);

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        modulatedOutBuffer.setSize (1, numSamples, false, false, true);
        nonModulatedOutBuffer.setSize (1, numSamples, false, false, true);

        BufferHelpers::collapseToMonoBuffer (audioInBuffer, nonModulatedOutBuffer);

        const auto* fbData = feedbackParam.getSmoothedBuffer();
        const auto* modDataPtr = modData.data();

        auto* modOutData = modulatedOutBuffer.getWritePointer (0);
        auto* noModOutData = nonModulatedOutBuffer.getWritePointer (0);

        fbStage.processStage (noModOutData, modOutData, modDataPtr, fbData, numSamples);
        modStages.processStage (modOutData, modOutData, modDataPtr, fbData, numSamples);
        fbStageNoMod.processStage (noModOutData, noModOutData, modDataPtr, fbData, numSamples);

        FloatVectorOperations::multiply (modOutData, modSmooth.getSmoothedBuffer(), numSamples);
        FloatVectorOperations::addWithMultiply (modOutData, noModOutData, noModSmooth.getSmoothedBuffer(), numSamples);
    }
    else
    {
        modulatedOutBuffer.setSize (1, numSamples, false, false, true);
        modulatedOutBuffer.clear();
        nonModulatedOutBuffer.setSize (1, numSamples, false, false, true);
        nonModulatedOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &modulatedOutBuffer;
    outputBuffers.getReference (Stage1Output) = &nonModulatedOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}

void Phaser8::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    modOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
    }
    else
    {
        modOutBuffer.clear();
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);

        modulatedOutBuffer.setSize (1, numSamples, false, false, true);
        nonModulatedOutBuffer.setSize (1, numSamples, false, false, true);

        BufferHelpers::collapseToMonoBuffer (audioInBuffer, nonModulatedOutBuffer);
        chowdsp::BufferMath::copyBufferData (nonModulatedOutBuffer, modulatedOutBuffer);
    }
    else
    {
        modulatedOutBuffer.setSize (1, numSamples, false, false, true);
        modulatedOutBuffer.clear();
        nonModulatedOutBuffer.setSize (1, numSamples, false, false, true);
        nonModulatedOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &modulatedOutBuffer;
    outputBuffers.getReference (Stage1Output) = &nonModulatedOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}
