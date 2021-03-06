#include "Rotary.h"
#include "../ParameterHelpers.h"

Rotary::Rotary (UndoManager* um) : BaseProcessor ("Rotary", createParameterLayout(), um)
{
    rateHzParam = vts.getRawParameterValue ("rate");

    auto* depthParamHandle = vts.getRawParameterValue ("depth");
    spectralDepthSmoothed.setParameterHandle (depthParamHandle);
    tremDepthSmoothed.setParameterHandle (depthParamHandle);
    chorusDepthSmoothed.setParameterHandle (depthParamHandle);

    uiOptions.backgroundColour = Colours::limegreen.darker (0.2f);
    uiOptions.powerColour = Colours::orange.brighter (0.1f);
    uiOptions.info.description = "A rotating speaker effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Rotary::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createFreqParameter (params, "rate", "Rate", 0.25f, 8.0f, 1.0f, 1.0f);
    createPercentParameter (params, "depth", "Depth", 0.5f);

    return { params.begin(), params.end() };
}

void Rotary::prepare (double sampleRate, int samplesPerBlock)
{
    const auto&& monoSpec = dsp::ProcessSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    modulator.prepare (monoSpec);
    modulationBuffer.setSize (2, samplesPerBlock);

    tremDepthSmoothed.prepare (sampleRate, samplesPerBlock);
    tremDepthSmoothed.mappingFunction = [] (float val)
    { return 0.85f * std::pow (val, 0.75f); };
    tremModData.resize ((size_t) samplesPerBlock);

    spectralDepthSmoothed.prepare (sampleRate, samplesPerBlock);
    spectralDepthSmoothed.mappingFunction = [this] (float val)
    { return std::pow (val, 0.8f) * (0.2f / std::pow (*rateHzParam, 1.25f)); };

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

void Rotary::processModulation (int numChannels, int numSamples)
{
    modulationBuffer.setSize (numChannels, numSamples, false, false, true);
    modulationBuffer.clear();
    auto&& monoBlock = dsp::AudioBlock<float> { modulationBuffer }.getSingleChannelBlock (0);

    modulator.setFrequency (*rateHzParam);
    modulator.process (dsp::ProcessContextReplacing<float> { monoBlock });

    if (numChannels > 1)
        FloatVectorOperations::negate (modulationBuffer.getWritePointer (1), modulationBuffer.getReadPointer (0), numSamples);
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
    const auto numChannels = buffer.getNumChannels();

    processModulation (numChannels, numSamples);
    tremDepthSmoothed.process (numSamples);
    spectralDepthSmoothed.process (numSamples);
    chorusDepthSmoothed.process (numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        const auto* modData = modulationBuffer.getReadPointer (ch);

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
