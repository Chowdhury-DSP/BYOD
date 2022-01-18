#include "ChainIOProcessor.h"
#include "../ParameterHelpers.h"

namespace
{
const String oversamplingTag = "oversampling";
const String inGainTag = "in_gain";
const String outGainTag = "out_gain";
const String dryWetTag = "dry_wet";
} // namespace

ChainIOProcessor::ChainIOProcessor (AudioProcessorValueTreeState& vts, std::function<void (int)>&& latencyChangedCallback) : latencyChangedCallbackFunc (std::move (latencyChangedCallback)),
                                                                                                                             oversampling (vts, 2, true)
{
    inGainParam = vts.getRawParameterValue (inGainTag);
    outGainParam = vts.getRawParameterValue (outGainTag);
    dryWetParam = vts.getRawParameterValue (dryWetTag);
}

void ChainIOProcessor::createParameters (Parameters& params)
{
    using OSFactor = chowdsp::VariableOversampling<float>::OSFactor;
    using OSMode = chowdsp::VariableOversampling<float>::OSMode;

    chowdsp::VariableOversampling<float>::createParameterLayout (params,
                                                                 { OSFactor::OneX, OSFactor::TwoX, OSFactor::FourX, OSFactor::EightX, OSFactor::SixteenX },
                                                                 { OSMode::MinPhase, OSMode::LinPhase },
                                                                 OSFactor::TwoX,
                                                                 OSMode::MinPhase);

    using namespace ParameterHelpers;
    createGainDBParameter (params, inGainTag, "In Gain", -72.0f, 18.0f, 0.0f, 0.0f);
    createGainDBParameter (params, outGainTag, "Out Gain", -72.0f, 18.0f, 0.0f, 0.0f);
    createPercentParameter (params, dryWetTag, "Dry/Wet", 1.0f);
}

void ChainIOProcessor::prepare (double sampleRate, int samplesPerBlock)
{
    oversampling.prepareToPlay (sampleRate, samplesPerBlock);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    for (auto* gain : { &inGain, &outGain })
    {
        gain->prepare (spec);
        gain->setRampDurationSeconds (0.1);
    }

    dryWetMixer.prepare (spec);
    latencyChangedCallbackFunc ((int) oversampling.getLatencySamples());
}

int ChainIOProcessor::getOversamplingFactor() const
{
    return oversampling.getOSFactor();
}

dsp::AudioBlock<float> ChainIOProcessor::processAudioInput (AudioBuffer<float>& buffer, bool& sampleRateChanged)
{
    if (oversampling.updateOSFactor())
    {
        sampleRateChanged = true;
        latencyChangedCallbackFunc ((int) oversampling.getLatencySamples());
    }

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    inGain.setGainDecibels (inGainParam->load());
    inGain.process (context);

    dryWetMixer.setDryWet (dryWetParam->load());
    dryWetMixer.copyDryBuffer (buffer);

    return oversampling.processSamplesUp (block);
}

void ChainIOProcessor::processAudioOutput (AudioBuffer<float>& buffer)
{
    dsp::AudioBlock<float> block (buffer);
    oversampling.processSamplesDown (block);

    const auto latencySamples = (int) oversampling.getLatencySamples();
    dryWetMixer.processBlock (buffer, latencySamples);

    dsp::ProcessContextReplacing<float> context (block);
    outGain.setGainDecibels (outGainParam->load());
    outGain.process (context);
}
