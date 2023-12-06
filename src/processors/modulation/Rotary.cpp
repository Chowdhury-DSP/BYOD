#include "Rotary.h"
#include "../BufferHelpers.h"
#include "../ParameterHelpers.h"

namespace RotaryTags
{
const String stereoTag = "stereo";
}

Rotary::Rotary (UndoManager* um) : BaseProcessor (
    "Rotary",
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
    chowdsp::ParamUtils::loadParameterPointer (rateHzParam, vts, "rate");
    chowdsp::ParamUtils::loadParameterPointer (stereoParam, vts, RotaryTags::stereoTag);

    addPopupMenuParameter (RotaryTags::stereoTag);

    auto* depthParamHandle = dynamic_cast<chowdsp::FloatParameter*> (vts.getParameter ("depth"));
    spectralDepthSmoothed.setParameterHandle (depthParamHandle);
    tremDepthSmoothed.setParameterHandle (depthParamHandle);
    chorusDepthSmoothed.setParameterHandle (depthParamHandle);

    uiOptions.backgroundColour = Colours::limegreen.darker (0.2f);
    uiOptions.powerColour = Colours::orange.brighter (0.1f);
    uiOptions.info.description = "A rotating speaker effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    disableWhenInputConnected ({ "rate" }, ModulationInput);
}

ParamLayout Rotary::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createFreqParameter (params, "rate", "Rate", 0.25f, 8.0f, 1.0f, 1.0f);
    createPercentParameter (params, "depth", "Depth", 0.5f);

    emplace_param<chowdsp::BoolParameter> (params, RotaryTags::stereoTag, "Stereo", false);

    return { params.begin(), params.end() };
}

void Rotary::prepare (double sampleRate, int samplesPerBlock)
{
    const auto&& monoSpec = dsp::ProcessSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    modulator.prepare (monoSpec);
    modulationBuffer.setSize (1, samplesPerBlock);
    modulationBufferNegative.setSize (1, samplesPerBlock);

    audioOutBuffer.setSize (2, samplesPerBlock);

    tremDepthSmoothed.mappingFunction = [] (float val)
    { return 0.85f * std::pow (val, 0.75f); };
    tremDepthSmoothed.setRampLength (0.05);
    tremDepthSmoothed.prepare (sampleRate, samplesPerBlock);
    tremModData.resize ((size_t) samplesPerBlock);

    spectralDepthSmoothed.mappingFunction = [this] (float val)
    { return std::pow (val, 0.8f) * (0.2f / std::pow (*rateHzParam, 1.25f)); };
    spectralDepthSmoothed.setRampLength (0.05);
    spectralDepthSmoothed.prepare (sampleRate, samplesPerBlock);

    chorusDepthSmoothed.prepare (sampleRate, samplesPerBlock);

    const auto numSpectralFilters = size_t (std::ceil (4.0 * std::sqrt (sampleRate / 48000.0)));
    for (auto& state : spectralFilterState)
        state.resize (numSpectralFilters, 0.0f);

    for (auto& mixer : chorusMixer)
    {
        mixer.prepare (monoSpec);
        mixer.setMixingRule (dsp::DryWetMixingRule::sin3dB);
    }

    for (auto& channelDelays : chorusDelay)
    {
        for (auto& delay : channelDelays)
        {
            delay.prepare (monoSpec);
            delay.setFilterFreq (12000.0f);
        }
    }

    chorusDepthSamples = 0.6f * 0.001f * (float) sampleRate;
}

void Rotary::processModulation (int numSamples)
{
    modulationBuffer.setSize (1, numSamples, false, false, true);
    modulationBuffer.clear();

    if (inputsConnected.contains (ModulationInput))
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modulationBuffer);
        FloatVectorOperations::clip (modulationBuffer.getWritePointer (0),
                                     modulationBuffer.getReadPointer (0),
                                     -1.0f,
                                     1.0f,
                                     numSamples);
    }
    else
    {
        auto&& monoBlock = dsp::AudioBlock<float> { modulationBuffer };
        modulator.setFrequency (*rateHzParam);
        modulator.process (dsp::ProcessContextReplacing<float> { monoBlock });
    }

    modulationBufferNegative.setSize (1, numSamples, false, false, true);
    modulationBufferNegative.clear();
    FloatVectorOperations::negate (modulationBufferNegative.getWritePointer (0), modulationBuffer.getReadPointer (0), numSamples);
}

void Rotary::processSpectralDelayFilters (int channel, float* data, const float* modData, const float* depthData, int numSamples)
{
    for (int n = 0; n < numSamples; ++n)
    {
        float modDepth = depthData[n] * 0.5f;
        float modBias = (1.0f - modDepth) * -0.965f;
        float m = modData[n] * modDepth + modBias;

        for (auto& state : spectralFilterState[channel])
        {
            float y = state + m * data[n];
            state = data[n] - m * y;

            data[n] = y;
        }
    }
}

void Rotary::processChorusing (int channel, float* data, const float* modData, const float* depthData, int numSamples)
{
    auto&& monoBlock = dsp::AudioBlock<float> { &data, 1, (size_t) numSamples };
    chorusMixer[channel].setWetMixProportion (depthData[numSamples - 1]);
    chorusMixer[channel].pushDrySamples (monoBlock);

    for (int n = 0; n < numSamples; ++n)
    {
        const auto xIn = data[n];
        data[n] = 0.0f;

        const auto chorusDepth = chorusDepthSamples * depthData[n];
        for (int i = 0; i < delaysPerChannel; ++i)
        {
            const auto modAmt = (i == 0 ? 0.85f : -0.85f) * modData[n];
            const auto delayAmt = chorusDepth * (1.0f + modAmt);

            chorusDelay[channel][i].setDelay (delayAmt);
            chorusDelay[channel][i].pushSample (0, xIn);
            data[n] += chorusDelay[channel][i].popSample (0);
        }
    }

    chorusMixer[channel].mixWetSamples (monoBlock);
}

void Rotary::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    processModulation (numSamples);
    tremDepthSmoothed.process (numSamples);
    spectralDepthSmoothed.process (numSamples);
    chorusDepthSmoothed.process (numSamples);

    if (inputsConnected.contains (AudioInput))
    {
        const auto stereoMode = stereoParam->get();
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numInChannels = audioInBuffer.getNumChannels();
        const auto numOutChannels = stereoMode ? 2 : numInChannels;

        audioOutBuffer.setSize (numOutChannels, numSamples, false, false, true);

        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            audioOutBuffer.copyFrom (ch, 0, audioInBuffer, ch % numInChannels, 0, numSamples);
            auto* x = audioOutBuffer.getWritePointer (ch);
            const auto* modData = ch == 0 ? modulationBuffer.getReadPointer (0) : modulationBufferNegative.getReadPointer (0);

            processSpectralDelayFilters (ch, x, modData, spectralDepthSmoothed.getSmoothedBuffer(), numSamples);
            processChorusing (ch, x, modData, chorusDepthSmoothed.getSmoothedBuffer(), numSamples);

            // tremolo
            const auto* tremDepth = tremDepthSmoothed.getSmoothedBuffer();
            FloatVectorOperations::multiply (tremModData.data(), modData, 0.5f, numSamples);
            FloatVectorOperations::add (tremModData.data(), 0.5f, numSamples);

            for (int n = 0; n < numSamples; ++n)
            {
                const auto modDepth = tremDepth[n];
                const auto m = tremModData[n] * modDepth + (1.0f - modDepth);
                x[n] *= m;
            }
        }
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modulationBuffer;
}

void Rotary::processAudioBypassed (AudioBuffer<float>& buffer)
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
    outputBuffers.getReference (ModulationOutput) = &modulationBuffer;
}
