#include "FreqBandSplitter.h"
#include "../ParameterHelpers.h"

FreqBandSplitter::FreqBandSplitter (UndoManager* um) : BaseProcessor ("Frequency Splitter",
                                                                      createParameterLayout(),
                                                                      BasicInputPort {},
                                                                      OutputPort {},
                                                                      um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (crossLowParam, vts, "cross_low");
    loadParameterPointer (crossHighParam, vts, "cross_high");

    uiOptions.backgroundColour = Colours::slategrey;
    uiOptions.powerColour = Colours::cyan;
    uiOptions.info.description = "Splits a signal into three frequency bands.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout FreqBandSplitter::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createFreqParameter (params, "cross_low", "Low Crossover", 20.0f, 2000.0f, 200.0f, 250.0f);
    createFreqParameter (params, "cross_high", "High Crossover", 200.0f, 20000.0f, 2000.0f, 1000.0f);

    return { params.begin(), params.end() };
}

void FreqBandSplitter::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };

    for (auto filt : { &lowCrossLPF1, &lowCrossLPF2, &highCrossLPF1, &highCrossLPF2 })
        filt->prepare (spec);

    for (auto filt : { &lowCrossHPF1, &lowCrossHPF2, &highCrossHPF1, &highCrossHPF2 })
        filt->prepare (spec);

    for (auto& b : buffers)
        b.setSize (2, samplesPerBlock);
}

void FreqBandSplitter::processAudio (AudioBuffer<float>& buffer)
{
    for (auto filt : { &lowCrossLPF1, &lowCrossLPF2 })
        filt->setCutoffFrequency (*crossLowParam);

    for (auto filt : { &lowCrossHPF1, &lowCrossHPF2 })
        filt->setCutoffFrequency (*crossLowParam);

    for (auto filt : { &highCrossLPF1, &highCrossLPF2 })
        filt->setCutoffFrequency (*crossHighParam);

    for (auto filt : { &highCrossHPF1, &highCrossHPF2 })
        filt->setCutoffFrequency (*crossHighParam);

    for (auto& b : buffers)
        b.makeCopyOf (buffer, true);

    // high band
    {
        dsp::AudioBlock<float> block { buffers[HighBand] };
        dsp::ProcessContextReplacing<float> ctx { block };
        highCrossHPF1.process (ctx);
        highCrossHPF2.process (ctx);
    }

    // mid band
    {
        dsp::AudioBlock<float> block { buffers[MidBand] };
        dsp::ProcessContextReplacing<float> ctx { block };
        lowCrossHPF1.process (ctx);
        lowCrossHPF2.process (ctx);
        highCrossLPF1.process (ctx);
        highCrossLPF2.process (ctx);
    }

    // low band
    {
        dsp::AudioBlock<float> block { buffers[LowBand] };
        dsp::ProcessContextReplacing<float> ctx { block };
        lowCrossLPF1.process (ctx);
        lowCrossLPF2.process (ctx);
    }

    for (int i = 0; i < numOuts; ++i)
        outputBuffers.getReference (i) = &buffers[i];
}

void FreqBandSplitter::processAudioBypassed (AudioBuffer<float>& buffer)
{
    for (auto& b : buffers)
        b.makeCopyOf (buffer, true);

    for (int i = 0; i < numOuts; ++i)
        outputBuffers.getReference (i) = &buffers[i];
}
