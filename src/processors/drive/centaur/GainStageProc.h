#pragma once

#include "AmpStage.h"
#include "ClippingStage.h"
#include "FeedForward2.h"
#include "PreAmpStage.h"
#include "SummingAmp.h"

class GainStageProcessor
{
public:
    GainStageProcessor() = default;

    void prepare (double sample_rate, int samples_per_block, int num_channels);
    void process (const chowdsp::BufferView<float>& buffer, float gain_param) noexcept;

    gain_stage::PreAmpWDF preamp_wdf[2];
    gain_stage::AmpStage amp_stage;
    gain_stage::ClippingStageWDF clipping_wdf[2];
    gain_stage::FeedForward2WDF ff2_wdf[2];
    gain_stage::SummingAmp summing_amp;

private:
    chowdsp::Buffer<float> ff1_buffer;
    chowdsp::Buffer<float> ff2_buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainStageProcessor)
};
