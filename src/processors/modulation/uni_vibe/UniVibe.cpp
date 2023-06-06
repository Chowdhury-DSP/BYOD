#include "UniVibe.h"
#include "processors/ParameterHelpers.h"
#include "processors/BufferHelpers.h"

namespace
{
const String speedTag = "speed";
const String intensityTag = "intensity";
const String mixTag = "mix";
}

// TODO:
// - fix DSP in its current form
// - more stage customization
// - variable number of stages
// - nonlinearities
// - circuit customization

UniVibe::UniVibe (UndoManager* um) : BaseProcessor ("Solo-Vibe",
                                                    createParameterLayout(),
                                                    um,
                                                    magic_enum::enum_count<InputPort>(),
                                                    magic_enum::enum_count<OutputPort>())
{
    using namespace ParameterHelpers;
    speedParamSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, speedTag));
    intensityParamSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, intensityTag));

    uiOptions.backgroundColour = Colours::darkgrey.darker (0.1f);
    uiOptions.powerColour = Colours::lightgrey.brighter();
    uiOptions.info.description = "A vibrato/chorus effect based on the Univox Uni-Vibe pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ speedTag }, ModulationInput);

    stages[0].alpha = 1.01f;
    stages[0].beta = 1.11f;
    stages[0].C_p = 0.015e-6f;
    stages[1].alpha = 0.98f;
    stages[1].beta = 1.09f;
    stages[1].C_p = 0.22e-6f;
    stages[2].alpha = 0.97f;
    stages[2].beta = 1.1f;
    stages[2].C_p = 470.0e-12f;
    stages[3].alpha = 0.95f;
    stages[3].beta = 1.09f;
    stages[3].C_p = 0.0047e-6f;

//    stages[1].ldrMap.A = -21.0e3f;
//    stages[1].ldrMap.B = 350.0e3f;
//    stages[1].ldrMap.C = 2.1f;
//    stages[2].ldrMap.A = -22.0e3f;
//    stages[2].ldrMap.B = 315.0e3f;
//    stages[2].ldrMap.C = 2.0f;
//    stages[3].ldrMap.A = -19.0e3f;
//    stages[3].ldrMap.B = 300.0e3f;
//    stages[3].ldrMap.C = 2.5f;
}

ParamLayout UniVibe::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createFreqParameter (params, speedTag, "Speed", 0.5f, 20.0f, 5.0f, 5.0f);
    createPercentParameter (params, intensityTag, "Intensity", 0.5f);
    createPercentParameter (params, mixTag, "Mix", 0.5f);

    return { params.begin(), params.end() };
}

void UniVibe::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec monoSpec { sampleRate, (uint32) samplesPerBlock, 1 };

    speedParamSmooth.setRampLength (0.01);
    speedParamSmooth.prepare (sampleRate, samplesPerBlock);
    intensityParamSmooth.setRampLength (0.01);
    intensityParamSmooth.prepare (sampleRate, samplesPerBlock);
    lfo.prepare (monoSpec);

    for (auto& stage : stages)
        stage.prepare (sampleRate, samplesPerBlock);

    modOutBuffer.setSize (1, samplesPerBlock);
    audioOutBuffer.setSize (2, samplesPerBlock);
}

void UniVibe::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    modOutBuffer.setSize (1, numSamples, false, false, true);
    modOutBuffer.clear();
    speedParamSmooth.process (numSamples);
    intensityParamSmooth.process (numSamples);

    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
    }
    else // create our own modulation signal
    {
        // fill modulation buffer (-1, 1)
        if (speedParamSmooth.isSmoothing())
        {
            const auto* speedParamSmoothData  = speedParamSmooth.getSmoothedBuffer();
            auto* modData = modOutBuffer.getWritePointer (0);
            for (int n = 0; n < numSamples; ++n)
            {
                lfo.setFrequency (speedParamSmoothData[n]);
                modData[n] = lfo.processSample();
            }
        }
        else
        {
            lfo.setFrequency (speedParamSmooth.getCurrentValue());
            lfo.processBlock (modOutBuffer);
        }
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);

        stages[0].process (audioInBuffer, audioOutBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer());
        for (int stageIdx = 0; stageIdx < (int) std::size (stages); ++stageIdx)
            stages[stageIdx].process (audioOutBuffer, audioOutBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer());
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}

void UniVibe::processAudioBypassed (AudioBuffer<float>& buffer)
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
