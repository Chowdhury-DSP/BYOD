#pragma once

#include <pch.h>

namespace BufferHelpers
{
/** Merges the signal from all the channels of the srcBuffer into the first channel of the destBuffer */
inline void collapseToMonoBuffer (const AudioBuffer<float>& srcBuffer, AudioBuffer<float>& destBuffer)
{
    jassert (srcBuffer.getNumSamples() == destBuffer.getNumSamples());
    const auto numSamples = srcBuffer.getNumSamples();

    if (&srcBuffer != &destBuffer)
        destBuffer.copyFrom (0, 0, srcBuffer, 0, 0, numSamples);

    if (const auto srcNumChannels = srcBuffer.getNumChannels(); srcNumChannels > 1)
    {
        for (int ch = 1; ch < srcNumChannels; ++ch)
            destBuffer.addFrom (0, 0, srcBuffer, ch, 0, numSamples);
        destBuffer.applyGain (1.0f / (float) srcNumChannels);
    }
}
} // namespace BufferHelpers
