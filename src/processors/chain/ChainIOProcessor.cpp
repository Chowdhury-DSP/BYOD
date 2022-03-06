#include "ChainIOProcessor.h"
#include "../ParameterHelpers.h"

namespace
{
const String monoModeTag = "mono_mode";
const String inGainTag = "in_gain";
const String outGainTag = "out_gain";
const String dryWetTag = "dry_wet";
} // namespace

ChainIOProcessor::ChainIOProcessor (AudioProcessorValueTreeState& vts, std::function<void (int)>&& latencyChangedCallback) : latencyChangedCallbackFunc (std::move (latencyChangedCallback)),
                                                                                                                             oversampling (vts, 2, true)
{
    monoModeParam = vts.getRawParameterValue (monoModeTag);
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
    params.push_back (std::make_unique<AudioParameterChoice> (monoModeTag, "Mode", StringArray { "Mono", "Stereo", "Left", "Right" }, 0));
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

    ioBuffer.setSize (2, samplesPerBlock);
    dryWetMixer.prepare (spec);
    latencyChangedCallbackFunc ((int) oversampling.getLatencySamples());
}

int ChainIOProcessor::getOversamplingFactor() const
{
    return oversampling.getOSFactor();
}

bool ChainIOProcessor::processChannelInputs (const AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    const auto useMono = monoModeParam->load() == 0.0f;
    const auto useStereo = monoModeParam->load() == 1.0f;
    const auto useLeft = monoModeParam->load() == 2.0f;
    const auto useRight = monoModeParam->load() == 3.0f;

    // we need this buffer to be stereo so that the oversampling processing is always stereo
    ioBuffer.setSize (2, numSamples, false, false, true);

    // simple case: stereo input!
    if (useStereo)
    {
        for (int ch = 0; ch < 2; ++ch)
            ioBuffer.copyFrom (ch, 0, buffer, ch % numChannels, 0, numSamples);
    }
    else
    {
        // Mono output, but which mono?
        ioBuffer.clear();
        if (useMono)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                ioBuffer.addFrom (0, 0, buffer, ch, 0, numSamples);

            ioBuffer.applyGain (1.0f / (float) numChannels);
        }
        else if (useLeft)
        {
            ioBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
        }
        else if (useRight)
        {
            ioBuffer.copyFrom (0, 0, buffer, 1 % numChannels, 0, numSamples);
        }

        ioBuffer.copyFrom (1, 0, ioBuffer, 0, 0, numSamples);
    }

    return useStereo;
}

dsp::AudioBlock<float> ChainIOProcessor::processAudioInput (const AudioBuffer<float>& buffer, bool& sampleRateChanged)
{
    if (oversampling.updateOSFactor())
    {
        sampleRateChanged = true;
        latencyChangedCallbackFunc ((int) oversampling.getLatencySamples());
    }

    const auto useStereo = processChannelInputs (buffer);

    auto&& block = dsp::AudioBlock<float> { ioBuffer };
    auto&& context = dsp::ProcessContextReplacing<float> { block };

    inGain.setGainDecibels (inGainParam->load());
    inGain.process (context);

    dryWetMixer.setDryWet (dryWetParam->load());
    dryWetMixer.copyDryBuffer (ioBuffer);

    processBlock = oversampling.processSamplesUp (block);

    if (useStereo)
        return processBlock; // return stereo block

    return processBlock.getSingleChannelBlock (0); // return mono block
}

void ChainIOProcessor::processChannelOutputs (AudioBuffer<float>& buffer, int numChannelsProcessed) const
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, ioBuffer, ch % numChannelsProcessed, 0, buffer.getNumSamples());
}

void ChainIOProcessor::processAudioOutput (const AudioBuffer<float>& processedBuffer, AudioBuffer<float>& outputBuffer)
{
    const auto numProcessedChannels = processedBuffer.getNumChannels();
    auto&& processedBlock = dsp::AudioBlock<const float> { processedBuffer };
    for (size_t ch = 0; ch < 2; ++ch)
        processBlock.getSingleChannelBlock (ch).copyFrom (processedBlock.getSingleChannelBlock (ch % (size_t) numProcessedChannels));

    auto&& outputBlock = dsp::AudioBlock<float> { ioBuffer };
    oversampling.processSamplesDown (outputBlock);

    const auto latencySamples = (int) oversampling.getLatencySamples();
    dryWetMixer.processBlock (ioBuffer, latencySamples);

    outGain.setGainDecibels (outGainParam->load());
    outGain.process (dsp::ProcessContextReplacing<float> { outputBlock });

    processChannelOutputs (outputBuffer, numProcessedChannels);
}
