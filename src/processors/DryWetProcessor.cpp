#include "DryWetProcessor.h"

void DryWetProcessor::prepare (const dsp::ProcessSpec& spec)
{
    dryBuffer.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);
    dryDelay.prepare (spec);
    reset();
}

void DryWetProcessor::reset()
{
    lastDryWet = dryWet;
}

void DryWetProcessor::copyDryBuffer (const AudioBuffer<float>& buffer)
{
    dryBuffer.makeCopyOf (buffer, true);
}

void DryWetProcessor::processBlock (AudioBuffer<float>& buffer, float latencySamples)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    dryDelay.setDelay (latencySamples);
    dsp::AudioBlock<float> dryBlock (dryBuffer);
    dsp::ProcessContextReplacing<float> dryContext (dryBlock);
    dryDelay.process (dryContext);

    if (lastDryWet == dryWet)
    {
        buffer.applyGain (dryWet);
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addFrom (ch, 0, dryBuffer.getReadPointer (ch), numSamples, (1.0f - dryWet));
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.applyGainRamp (ch, 0, numSamples, lastDryWet, dryWet);
            buffer.addFromWithRamp (ch, 0, dryBuffer.getReadPointer (ch), numSamples, (1.0f - lastDryWet), (1.0f - dryWet));
        }

        lastDryWet = dryWet;
    }
}
