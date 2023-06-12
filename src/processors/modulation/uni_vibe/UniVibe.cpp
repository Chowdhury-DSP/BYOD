#include "UniVibe.h"
#include "processors/BufferHelpers.h"
#include "processors/ParameterHelpers.h"

namespace
{
const String speedTag = "speed";
const String intensityTag = "intensity";
const String numStagesTag = "num_stages";
const String stereoTag = "stereo";
const String mixTag = "mix";
} // namespace

UniVibe::UniVibe (UndoManager* um) : BaseProcessor ("Solo-Vibe",
                                                    createParameterLayout(),
                                                    um,
                                                    magic_enum::enum_count<InputPort>(),
                                                    magic_enum::enum_count<OutputPort>())
{
    using namespace ParameterHelpers;
    speedParamSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, speedTag));
    intensityParamSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, intensityTag));
    loadParameterPointer (numStagesParam, vts, numStagesTag);
    loadParameterPointer (stereoParam, vts, stereoTag);
    loadParameterPointer (mixParam, vts, mixTag);

    uiOptions.backgroundColour = Colours::darkgrey.darker (0.1f);
    uiOptions.powerColour = Colours::violet.brighter();
    uiOptions.info.description = "A vibrato/chorus effect based on the Univox Uni-Vibe pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ speedTag }, ModulationInput);
    addPopupMenuParameter (stereoTag);

    juce::Random rand { 0x1234321 };
    const auto randInRange = [&rand] (const juce::Range<float>& range)
    {
        return juce::jmap (rand.nextFloat(), range.getStart(), range.getEnd());
    };
    const auto randInLogRange = [&rand] (const juce::Range<float>& range)
    {
        return range.getStart() * std::pow (range.getEnd() / range.getStart(), rand.nextFloat());
    };

    for (auto& stage : stages)
    {
        stage.alpha = randInRange ({ 0.9f, 1.01f });
        stage.beta = randInRange ({ 1.0f, 1.1f });
        stage.C_p = randInLogRange ({ 100.0e-12f, 1.0e-6f });
        stage.ldrMap.A = randInRange ({ -22.0e3f, -18.0e3f });
        stage.ldrMap.B = randInRange ({ 300.0e3f, 350.0e3f });
        stage.ldrMap.C = randInRange ({ 2.0f, 2.5f });
    }
}

ParamLayout UniVibe::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createFreqParameter (params, speedTag, "Speed", 0.5f, 20.0f, 5.0f, 5.0f);
    createPercentParameter (params, intensityTag, "Intensity", 0.5f);
    emplace_param<chowdsp::FloatParameter> (
        params,
        numStagesTag,
        "# Stages",
        NormalisableRange { 1.0f, (float) maxNumStages },
        4,
        [] (float val)
        {
            return String { (int) val };
        },
        &stringToFloatVal);
    emplace_param<chowdsp::BoolParameter> (params, stereoTag, "Stereo", false);
    createPercentParameter (params, mixTag, "Mix", 0.5f);

    return { params.begin(), params.end() };
}

void UniVibe::prepare (double sampleRate, int samplesPerBlock)
{
    const auto monoSpec = dsp::ProcessSpec { sampleRate, (uint32) samplesPerBlock, 1 };

    speedParamSmooth.setRampLength (0.01);
    speedParamSmooth.prepare (sampleRate, samplesPerBlock);
    intensityParamSmooth.setRampLength (0.01);
    intensityParamSmooth.prepare (sampleRate, samplesPerBlock);
    lfo.prepare (monoSpec);

    for (auto& stage : stages)
        stage.prepare (sampleRate, samplesPerBlock);

    dryWetMixer.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    dryWetMixer.setMixingRule (juce::dsp::DryWetMixingRule::sin3dB);
    dryWetMixerMono.prepare (monoSpec);
    dryWetMixerMono.setMixingRule (juce::dsp::DryWetMixingRule::sin3dB);

    modOutBuffer.setSize (1, samplesPerBlock);
    audioOutBuffer.setSize (2, samplesPerBlock);
    fadeBuffer.setSize (2, samplesPerBlock);

    prevNumStages = (int) *numStagesParam;
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
            const auto* speedParamSmoothData = speedParamSmooth.getSmoothedBuffer();
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
        const auto useStereoMode = stereoParam->get();
        const auto [audioInBuffer, numChannels] = [this, numSamples, useStereoMode] () -> std::pair<const AudioBuffer<float>, int>
        {
            const auto& inBuffer = getInputBuffer (AudioInput);
            const auto nChannels = inBuffer.getNumChannels();
            if (! useStereoMode || nChannels == 2)
            {
                audioOutBuffer.setSize (nChannels, numSamples, false, false, true);
                audioOutBuffer.clear();
                return { inBuffer, nChannels };
            }

            audioOutBuffer.setSize (2, numSamples, false, false, true);
            audioOutBuffer.clear();
            for (int ch = 0; ch < 2; ++ch)
                audioOutBuffer.copyFrom (ch, 0, inBuffer.getReadPointer (0), numSamples);
            return { audioOutBuffer, 2 };
        }();

        auto& dryWet = numChannels == 1 ? dryWetMixerMono : dryWetMixer;
        dryWet.pushDrySamples (audioInBuffer);

        const auto numStagesToProcess = (int) *numStagesParam;
        if (numStagesToProcess == prevNumStages)
        {
            stages[0].process (audioInBuffer, audioOutBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
            for (int stageIdx = 1; stageIdx < numStagesToProcess; ++stageIdx)
                stages[stageIdx].process (audioOutBuffer, audioOutBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
        }
        else
        {
            fadeBuffer.setSize (numChannels, numSamples, false, false, true);
            if (numStagesToProcess > prevNumStages)
            {
                stages[0].process (audioInBuffer, fadeBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
                for (int stageIdx = 1; stageIdx < prevNumStages; ++stageIdx)
                    stages[stageIdx].process (fadeBuffer, fadeBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
                stages[prevNumStages].process (fadeBuffer, audioOutBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
                for (int stageIdx = prevNumStages + 1; stageIdx < numStagesToProcess; ++stageIdx)
                    stages[stageIdx].process (audioOutBuffer, audioOutBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
            }
            else if (numStagesToProcess < prevNumStages)
            {
                stages[0].process (audioInBuffer, audioOutBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
                for (int stageIdx = 1; stageIdx < numStagesToProcess; ++stageIdx)
                    stages[stageIdx].process (audioOutBuffer, audioOutBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
                stages[numStagesToProcess].process (audioOutBuffer, fadeBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
                for (int stageIdx = prevNumStages + 1; stageIdx < prevNumStages; ++stageIdx)
                {
                    stages[stageIdx].process (fadeBuffer, fadeBuffer, modOutBuffer.getReadPointer (0), intensityParamSmooth.getSmoothedBuffer(), useStereoMode);
                    stages[stageIdx].reset();
                }
            }

            audioOutBuffer.applyGainRamp (0, numSamples, 0.0f, 1.0f);
            for (int ch = 0; ch < numChannels; ++ch)
                audioOutBuffer.addFromWithRamp (ch, 0, fadeBuffer.getReadPointer (ch), numSamples, 1.0f, 0.0f);

            prevNumStages = numStagesToProcess;
        }

        dryWet.setWetMixProportion (*mixParam);
        dryWet.mixWetSamples (audioOutBuffer);
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
