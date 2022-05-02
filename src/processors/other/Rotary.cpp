#include "Rotary.h"
#include "../ParameterHelpers.h"

Rotary::Rotary (UndoManager* um) : BaseProcessor ("Rotary", createParameterLayout(), um)
{
    rateHzParam = vts.getRawParameterValue ("rate");
    depthSmoothed.setParameterHandle (vts.getRawParameterValue ("depth"));

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
    modulator.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });
    modulationBuffer.setSize (2, samplesPerBlock);

    depthSmoothed.prepare (sampleRate, samplesPerBlock);
    depthSmoothed.mappingFunction = [this] (float val)
    { return val * (0.25f / *rateHzParam); };

    const auto numSpectralFilters = size_t (std::ceil (4.0 * std::sqrt (sampleRate / 48000.0)));
    for (auto& state : spectralFilterState)
        //        std::fill (state.begin(), state.end(), 0.0f);
        state.resize (numSpectralFilters, 0.0f);

    {
        constexpr float maxFreq = 14000.0f;
        const auto RC_inv = MathConstants<float>::twoPi * maxFreq;
        const auto K = 2.0f * (float) sampleRate; // chowdsp::Bilinear::computeKValue (maxFreq, (float) sampleRate);
        const auto m_p = 1.0f + K / RC_inv;
        const auto m_n = 1.0f - K / RC_inv;
        maxSpectralCoeffMod = -1.0f - m_n / m_p;
    }
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

void Rotary::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    processModulation (numChannels, numSamples);
    depthSmoothed.process (numSamples);

    const auto* depthSmoothBuffer = depthSmoothed.getSmoothedBuffer();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        const auto* modData = modulationBuffer.getReadPointer (ch);

        processSpectralDelayFilters (ch, x, modData, depthSmoothBuffer, numSamples);

        // tremolo
        for (int n = 0; n < numSamples; ++n)
        {
            const auto modDepth = depthSmoothBuffer[n] * 0.25f;
            const auto m = modData[n] * modDepth + (1.0f - modDepth * 2.0f);
            x[n] *= m;
        }
    }
}
