#include "GainStageProc.h"

void GainStageProcessor::prepare (double sample_rate, int samples_per_block, int num_channels)
{
    ff1_buffer.setMaxSize (num_channels, samples_per_block);
    ff2_buffer.setMaxSize (num_channels, samples_per_block);

    for (auto& wdf : preamp_wdf)
        wdf.prepare ((float) sample_rate, samples_per_block);

    amp_stage.prepare ((float) sample_rate, samples_per_block, num_channels);

    for (auto& wdf : clipping_wdf)
        wdf.prepare ((float) sample_rate);

    for (auto& wdf : ff2_wdf)
        wdf.prepare ((float) sample_rate, samples_per_block);

    summing_amp.prepare ((float) sample_rate, num_channels);
}

void GainStageProcessor::process (const chowdsp::BufferView<float>& buffer, float gain_param) noexcept
{
    const auto num_channels = buffer.getNumChannels();
    const auto num_samples = buffer.getNumSamples();

    ff1_buffer.setCurrentSize (num_channels, num_samples);
    ff2_buffer.setCurrentSize (num_channels, num_samples);

    // side chain buffers
    chowdsp::BufferMath::copyBufferData (buffer, ff2_buffer);

    // Gain stage
    for (int ch = 0; ch < num_channels; ++ch)
        preamp_wdf[ch].process (buffer.getWriteSpan (ch), ff1_buffer.getWriteSpan (ch), gain_param);

    // amp stage
    amp_stage.process_block (buffer, gain_param);
    for (int ch = 0; ch < num_channels; ++ch)
        juce::FloatVectorOperations::clip (buffer.getWritePointer (ch), buffer.getReadPointer (ch), -4.5f, 4.5f, num_samples);

    // clipping stage
    for (int ch = 0; ch < num_channels; ++ch)
        clipping_wdf[ch].process (buffer.getWriteSpan (ch));

    // feed-forward network 2
    for (int ch = 0; ch < num_channels; ++ch)
        ff2_wdf[ch].process (ff2_buffer.getWriteSpan (ch), gain_param);

    // summing amp
    chowdsp::BufferMath::addBufferData (ff1_buffer, buffer);
    chowdsp::BufferMath::addBufferData (ff2_buffer, buffer);
    summing_amp.processBlock (buffer);
    for (int ch = 0; ch < num_channels; ++ch)
        juce::FloatVectorOperations::clip (buffer.getWritePointer (ch), buffer.getReadPointer (ch), -13.1f, 11.7f, num_samples);
}
