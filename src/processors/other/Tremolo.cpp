#include "Tremolo.h"

namespace
{
template <typename SampleType, typename SmoothingType>
static dsp::AudioBlock<SampleType>& addSmoothed (dsp::AudioBlock<SampleType>& block, SmoothedValue<SampleType, SmoothingType>& value) noexcept
{
    if (! value.isSmoothing())
        return block.add (value.getTargetValue());

    for (size_t i = 0; i < block.getNumSamples(); ++i)
    {
        const auto scaler = value.getNextValue();
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            block.getChannelPointer (ch)[i] += scaler;
    }

    return block;
}
} // namespace

Tremolo::Tremolo (UndoManager* um) : BaseProcessor ("Tremolo", createParameterLayout(), um, 2, 2)
{
    using namespace ParameterHelpers;
    loadParameterPointer (rateParam, vts, "rate");
    loadParameterPointer (waveParam, vts, "wave");
    loadParameterPointer (depthParam, vts, "depth");

    uiOptions.backgroundColour = Colours::orange.darker (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter();
    uiOptions.info.description = "A simple tremolo effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
}

ParamLayout Tremolo::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createFreqParameter (params, "rate", "Rate", 2.0f, 20.0f, 10.0f, 10.0f);
    createPercentParameter (params, "wave", "Wave", 0.5f);
    createPercentParameter (params, "depth", "Depth", 0.5f);

    return { params.begin(), params.end() };
}

void Tremolo::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec monoSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    sine.prepare (monoSpec);

    filter.prepare (monoSpec);
    filter.setCutoffFrequency (250.0f);

    waveBuffer.setSize (1, samplesPerBlock);
    modBuffer.setSize (1, samplesPerBlock);
    phaseSmooth.reset (sampleRate, 0.01);
    waveSmooth.reset (sampleRate, 0.01);
    depthGainSmooth.reset (sampleRate, 0.01);
    depthAddSmooth.reset (sampleRate, 0.01);

    fs = (float) sampleRate;
    phase = 0.0f;
}

void Tremolo::fillWaveBuffer (float* waveBuff, const int numSamples, float& p)
{
    bool isSmoothing = phaseSmooth.isSmoothing() || waveSmooth.isSmoothing();

    if (isSmoothing)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            auto curWave = waveSmooth.getNextValue();
            auto sineGain = 1.0f - jmin (2.0f * curWave, 1.0f);
            auto squareGain = jmax (2.0f * (curWave - 0.5f), 0.0f);
            auto triGain = 1.0f - 2.0f * std::abs (0.5f - curWave);

            waveBuff[n] = sineGain * dsp::FastMathApproximations::sin (p); // sine
            waveBuff[n] += triGain * p / MathConstants<float>::pi; // triangle
            waveBuff[n] += squareGain * (p > 0.0f ? 1.0f : -1.0f); // square

            p += phaseSmooth.getNextValue();
            p = p > MathConstants<float>::pi ? p - MathConstants<float>::twoPi : p;
        }
    }
    else
    {
        auto phaseInc = phaseSmooth.getNextValue();
        auto curWave = waveSmooth.getNextValue();
        if (curWave <= 0.5f)
        {
            auto sineGain = 1.0f - jmin (2.0f * curWave, 1.0f);
            auto triGain = 1.0f - 2.0f * std::abs (0.5f - curWave);
            for (int n = 0; n < numSamples; ++n)
            {
                waveBuff[n] = sineGain * dsp::FastMathApproximations::sin (p); // sine
                waveBuff[n] += triGain * p / MathConstants<float>::pi; // triangle

                p += phaseInc;
                p = p > MathConstants<float>::pi ? p - MathConstants<float>::twoPi : p;
            }
        }
        else
        {
            auto squareGain = jmax (2.0f * (curWave - 0.5f), 0.0f);
            auto triGain = 1.0f - 2.0f * std::abs (0.5f - curWave);
            for (int n = 0; n < numSamples; ++n)
            {
                waveBuff[n] = triGain * p / MathConstants<float>::pi; // triangle
                waveBuff[n] += squareGain * (p > 0.0f ? 1.0f : -1.0f); // square

                p += phaseInc;
                p = p > MathConstants<float>::pi ? p - MathConstants<float>::twoPi : p;
            }
        }
    }
}

void Tremolo::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    waveBuffer.setSize (1, numSamples, false, false, true);
    modBuffer.setSize (1, numSamples, false, false, true);
    phaseSmooth.setTargetValue (*rateParam * MathConstants<float>::pi / fs);
    waveSmooth.setTargetValue (*waveParam);

    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        modBuffer.copyFrom (0, 0, modInputBuffer, 0, 0, numSamples);

        if (const auto modInputNumChannels = modInputBuffer.getNumChannels(); modInputNumChannels > 1)
        {
            for (int ch = 1; ch < modInputNumChannels; ++ch)
                modBuffer.addFrom (0, 0, modInputBuffer, ch, 0, numSamples);
            modBuffer.applyGain (1.0f / (float) modInputNumChannels);
        }
    }
    else // create our own modulation signal
    {
        // fill modulation buffer (-1, 1)
        fillWaveBuffer (modBuffer.getWritePointer (0), numSamples, phase);
    }

    if (inputsConnected.contains (AudioInput))
    {
        // copy modulation buffer to wave buffer (-1, 1)
        waveBuffer.makeCopyOf (modBuffer, true);
        dsp::AudioBlock<float> waveBlock { waveBuffer };
        dsp::ProcessContextReplacing<float> waveCtx { waveBlock };

        // shrink range to (0, 1)
        waveBlock *= 0.5f;
        waveBlock += 0.5f;

        // apply depth parameter
        auto depthVal = std::pow (depthParam->load(), 0.33f);
        depthGainSmooth.setTargetValue (depthVal);
        waveBlock.multiplyBy (depthGainSmooth);
        depthAddSmooth.setTargetValue (1.0f - depthVal);
        addSmoothed (waveBlock, depthAddSmooth);
        filter.process (waveCtx);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* x = getInputBuffer (AudioInput).getWritePointer (0);
            FloatVectorOperations::multiply (x, x, waveBuffer.getReadPointer (0), numSamples);
        }
    }

    outputBuffers.getReference (AudioOutput) = &getInputBuffer (AudioInput);
    outputBuffers.getReference (ModulationOutput) = &modBuffer;
}
