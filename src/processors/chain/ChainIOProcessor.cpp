#include "ChainIOProcessor.h"
#include "../ParameterHelpers.h"

namespace
{
const String oversamplingTag = "oversampling";
const String inGainTag = "in_gain";
const String outGainTag = "out_gain";
const String dryWetTag = "dry_wet";
} // namespace

ChainIOProcessor::ChainIOProcessor (AudioProcessorValueTreeState& vts)
{
    oversamplingParam = vts.getRawParameterValue (oversamplingTag);
    inGainParam = vts.getRawParameterValue (inGainTag);
    outGainParam = vts.getRawParameterValue (outGainTag);
    dryWetParam = vts.getRawParameterValue (dryWetTag);

    for (int i = 0; i < 5; ++i)
        overSample[i] = std::make_unique<dsp::Oversampling<float>> (2, i, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
}

void ChainIOProcessor::createParameters (Parameters& params)
{
    params.push_back (std::make_unique<AudioParameterChoice> (oversamplingTag,
                                                              "Oversampling",
                                                              StringArray { "1x", "2x", "4x", "8x", "16x" },
                                                              1));

    using namespace ParameterHelpers;
    createGainDBParameter (params, inGainTag, "In Gain", -72.0f, 18.0f, 0.0f);
    createGainDBParameter (params, outGainTag, "Out Gain", -72.0f, 18.0f, 0.0f);
    createPercentParameter (params, dryWetTag, "Dry/Wet", 1.0f);
}

void ChainIOProcessor::prepare (double sampleRate, int samplesPerBlock)
{
    int curOS = static_cast<int> (*oversamplingParam);
    prevOS = curOS;
    for (int i = 0; i < 5; ++i)
        overSample[i]->initProcessing ((size_t) samplesPerBlock);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    for (auto* gain : { &inGain, &outGain })
    {
        gain->prepare (spec);
        gain->setRampDurationSeconds (0.1);
    }

    dryWetMixer.prepare (spec);
}

int ChainIOProcessor::getOversamplingFactor() const
{
    int curOS = static_cast<int> (*oversamplingParam);
    return (int) overSample[curOS]->getOversamplingFactor();
}

dsp::AudioBlock<float> ChainIOProcessor::processAudioInput (AudioBuffer<float>& buffer, bool& sampleRateChanged)
{
    int curOS = static_cast<int> (*oversamplingParam);
    if (prevOS != curOS)
    {
        sampleRateChanged = true;
        prevOS = curOS;
    }

    // set input, output, and dry/wet
    inGain.setGainDecibels (inGainParam->load());
    outGain.setGainDecibels (outGainParam->load());
    dryWetMixer.setDryWet (dryWetParam->load());

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);
    inGain.process (context);
    dryWetMixer.copyDryBuffer (buffer);

    return overSample[curOS]->processSamplesUp (block);
}

void ChainIOProcessor::processAudioOutput (AudioBuffer<float>& buffer)
{
    int curOS = static_cast<int> (*oversamplingParam);
    dsp::AudioBlock<float> block (buffer);
    overSample[curOS]->processSamplesDown (block);

    auto latencySamples = overSample[curOS]->getLatencyInSamples();
    dryWetMixer.processBlock (buffer, latencySamples);

    dsp::ProcessContextReplacing<float> context (block);
    outGain.process (context);
}
