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

Phaser8::Phaser8 (UndoManager* um) : BaseProcessor ("Phaser8",
                                                    createParameterLayout(),
                                                    um,
                                                    magic_enum::enum_count<InputPort>(),
                                                    magic_enum::enum_count<OutputPort>())
{
    using namespace ParameterHelpers;
    loadParameterPointer (rateHzParam, vts, rateTag);

    depthParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, depthTag));
    depthParam.mappingFunction = [] (float x)
    { return 0.45f * x; };

    feedbackParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, feedbackTag));
    feedbackParam.mappingFunction = [] (float x)
    { return 0.95f * x; };

    modLeftSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, modulationTag));
    modLeftSmooth.mappingFunction = [] (float x)
    { return x > 0.0f ? x : 0.0f; };

    noModLeftSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, modulationTag));
    noModLeftSmooth.mappingFunction = [] (float x)
    { return x > 0.0f ? 1.0f - x : 1.0f; };

    modRightSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, modulationTag));
    modRightSmooth.mappingFunction = [] (float x)
    { return x < 0.0f ? -x : 0.0f; };

    noModRightSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, modulationTag));
    noModRightSmooth.mappingFunction = [] (float x)
    { return x < 0.0f ? 1.0f + x : 1.0f; };

    lfoShaper.initialise ([] (float x)
                          {
                              static constexpr auto skewFactor = gcem::pow (2.0f, -0.85f);
                              return 2.0f * std::pow ((x + 1.0f) * 0.5f, skewFactor) - 1.0f; },
                          -1.0f,
                          1.0f,
                          2048);

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ rateTag }, ModulationInput);

    uiOptions.backgroundColour = Colour { 0xff00a8e9 };
    uiOptions.powerColour = Colour { 0xfff44e44 };
    uiOptions.info.description = "A phaser effect based on a classic \"Compact\" 8-stage phaser pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Phaser8::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createFreqParameter (params, rateTag, "Rate", 0.05f, 20.0f, 0.5f, 0.5f);
    createPercentParameter (params, depthTag, "Depth", 1.0f);
    createBipolarPercentParameter (params, feedbackTag, "Feedback", 0.5f);
    createBipolarPercentParameter (params, modulationTag, "Modulation", 1.0f);

    return { params.begin(), params.end() };
}

void Phaser8::prepare (double sampleRate, int samplesPerBlock)
{
    depthParam.prepare (sampleRate, samplesPerBlock);
    depthParam.setRampLength (0.05);
    feedbackParam.prepare (sampleRate, samplesPerBlock);
    feedbackParam.setRampLength (0.05);
    modLeftSmooth.prepare (sampleRate, samplesPerBlock);
    modLeftSmooth.setRampLength (0.05);
    noModLeftSmooth.prepare (sampleRate, samplesPerBlock);
    noModLeftSmooth.setRampLength (0.05);
    modRightSmooth.prepare (sampleRate, samplesPerBlock);
    modRightSmooth.setRampLength (0.05);
    noModRightSmooth.prepare (sampleRate, samplesPerBlock);
    noModRightSmooth.setRampLength (0.05);

    fbStage.prepare ((float) sampleRate);
    fbStageNoMod.prepare ((float) sampleRate);
    modStages.prepare ((float) sampleRate);

    sineLFO.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 1 });
    modData.resize ((size_t) samplesPerBlock, 0.0f);

    modulatedOutBuffer.setSize (1, samplesPerBlock);
    nonModulatedOutBuffer.setSize (1, samplesPerBlock);

    audioOutBuffer.setSize (2, samplesPerBlock);
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
    modLeftSmooth.process (numSamples);
    noModLeftSmooth.process (numSamples);
    modRightSmooth.process (numSamples);
    noModRightSmooth.process (numSamples);
    processModulation (numSamples);

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        modulatedOutBuffer.setSize (2, numSamples, false, false, true); // always stereo out
        nonModulatedOutBuffer.setSize (2, numSamples, false, false, true); // always stereo out
        audioOutBuffer.setSize (2, numSamples, false, false, true); // always stereo out

        BufferHelpers::collapseToMonoBuffer (audioInBuffer, nonModulatedOutBuffer);

        const auto* fbData = feedbackParam.getSmoothedBuffer();
        const auto* modDataPtr = modData.data();

        auto* modOutData = modulatedOutBuffer.getWritePointer (0);
        auto* noModOutData = nonModulatedOutBuffer.getWritePointer (0);

        fbStage.processStage (noModOutData, modOutData, modDataPtr, fbData, numSamples);
        modStages.processStage (modOutData, modOutData, modDataPtr, fbData, numSamples);
        fbStageNoMod.processStage (noModOutData, noModOutData, modDataPtr, fbData, numSamples);

        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* modMultiply = ch == 0 ? modLeftSmooth.getSmoothedBuffer() : modRightSmooth.getSmoothedBuffer();
            const auto* nonModMultiply = ch == 0 ? noModLeftSmooth.getSmoothedBuffer() : noModRightSmooth.getSmoothedBuffer();

            FloatVectorOperations::multiply (audioOutBuffer.getWritePointer (ch), modOutData, modMultiply, numSamples);
            FloatVectorOperations::addWithMultiply (audioOutBuffer.getWritePointer (ch), noModOutData, nonModMultiply, numSamples);
        }
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
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
        const auto numChannels = audioInBuffer.getNumChannels();
        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);
        audioOutBuffer.makeCopyOf (audioInBuffer, true);
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}
