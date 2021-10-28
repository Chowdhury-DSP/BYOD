#include "EnvelopeFilter.h"
#include "../ParameterHelpers.h"

namespace
{
constexpr int mSize = 16;

constexpr float highQ = 5.0f;
constexpr float lowQ = 1.0f / MathConstants<float>::sqrt2;
static float getQ (float param01)
{
    return lowQ * std::pow ((highQ / lowQ), param01);
}

static NormalisableRange<float> speedRange (5.0f, 100.0f);
} // namespace

EnvelopeFilter::EnvelopeFilter (UndoManager* um) : BaseProcessor ("Envelope Filter", createParameterLayout(), um)
{
    freqParam = vts.getRawParameterValue ("freq");
    resParam = vts.getRawParameterValue ("res");
    senseParam = vts.getRawParameterValue ("sense");
    speedParam = vts.getRawParameterValue ("speed");
    filterTypeParam = vts.getRawParameterValue ("filter_type");

    speedRange.setSkewForCentre (20.0f);

    uiOptions.backgroundColour = Colours::purple.brighter();
    uiOptions.powerColour = Colours::yellow.darker (0.1f);
    uiOptions.info.description = "A envelope filter with lowpass, bandpass, and highpass filter types.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout EnvelopeFilter::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "res", "Resonance", 0.5f);
    createFreqParameter (params, "freq", "Freq.", 100.0f, 1000.0f, 250.0f, 250.0f);
    createPercentParameter (params, "speed", "Speed", 0.5f);
    createPercentParameter (params, "sense", "Sensitivity", 0.5f);

    params.push_back (std::make_unique<AudioParameterChoice> ("filter_type",
                                                              "Type",
                                                              StringArray { "Lowpass", "Bandpass", "Highpass" },
                                                              0));

    return { params.begin(), params.end() };
}

void EnvelopeFilter::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    auto monoSpec = spec;
    monoSpec.numChannels = 1;

    filter.prepare (spec);
    level.prepare (monoSpec);

    levelBuffer.setSize (1, samplesPerBlock);
}

template <chowdsp::StateVariableFilterType FilterType, typename ModFreqFunc>
void processFilter (AudioBuffer<float>& buffer, chowdsp::StateVariableFilter<float>& filter, ModFreqFunc&& getModFreq)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        int i = 0;
        for (int n = 0; n < numSamples - mSize; n += mSize)
        {
            filter.setCutoffFrequency (getModFreq (i));
            for (; i < n + mSize; ++i)
                x[i] = filter.processSample<FilterType> (ch, x[i]);
        }

        // process leftover samples
        filter.setCutoffFrequency (getModFreq (i));
        for (; i < numSamples; ++i)
            x[i] = filter.processSample<FilterType> (ch, x[i]);
    }
}

void EnvelopeFilter::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels == 1)
    {
        levelBuffer.setSize (1, numSamples, false, false, true);
        levelBuffer.clear();
        levelBuffer.copyFrom (0, 0, buffer.getReadPointer (0), numSamples);
    }
    else
    {
        levelBuffer.setSize (1, numSamples, false, false, true);
        levelBuffer.clear();

        levelBuffer.copyFrom (0, 0, buffer.getReadPointer (0), numSamples);

        for (int ch = 1; ch < numChannels; ++ch)
            levelBuffer.addFrom (0, 0, buffer.getReadPointer (ch), numSamples);

        levelBuffer.applyGain (1.0f / (float) numChannels);
    }

    auto speed = speedRange.convertFrom0to1 (1.0f - *speedParam);
    level.setParameters (speed, speed * 4.0f);
    dsp::AudioBlock<float> levelBlock { levelBuffer };
    dsp::ProcessContextReplacing<float> levelCtx { levelBlock };
    level.process (levelCtx);

    filter.setResonance (getQ (resParam->load()));
    auto filterFreqHz = freqParam->load();
    auto freqModGain = 20.0f * senseParam->load();
    auto* levelPtr = levelBuffer.getReadPointer (0);
    FloatVectorOperations::clip (levelBuffer.getWritePointer (0), levelPtr, 0.0f, 2.0f, numSamples);
    auto getModFreq = [=] (int i) -> float
    {
        return jlimit (20.0f, 20.0e3f, filterFreqHz + freqModGain * levelPtr[i] * filterFreqHz);
    };

    auto filterType = (int) filterTypeParam->load();
    switch (filterType)
    {
        case 0:
            processFilter<chowdsp::StateVariableFilterType::Lowpass> (buffer, filter, getModFreq);
            break;

        case 1:
            processFilter<chowdsp::StateVariableFilterType::Bandpass> (buffer, filter, getModFreq);
            break;

        case 2:
            processFilter<chowdsp::StateVariableFilterType::Highpass> (buffer, filter, getModFreq);
            break;

        default:
            jassertfalse;
    };

    buffer.applyGain (Decibels::decibelsToGain (-6.0f));
}
