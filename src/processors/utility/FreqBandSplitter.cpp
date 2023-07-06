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
    crossover.prepare (spec);

    for (auto& b : buffers)
        b.setSize (2, samplesPerBlock);
}

void FreqBandSplitter::processAudio (AudioBuffer<float>& buffer)
{
    for (auto& b : buffers)
        b.setSize (buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);

    crossover.setLowCrossoverFrequency (*crossLowParam);
    crossover.setHighCrossoverFrequency (*crossHighParam);
    crossover.processBlock (buffer, buffers[LowBand], buffers[MidBand], buffers[HighBand]);

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

String FreqBandSplitter::getTooltipForPort (int portIndex, bool isInput)
{
    if (! isInput)
    {
        switch ((OutputPort) portIndex)
        {
            case OutputPort::HighBand:
                return "High Band Output";
            case OutputPort::MidBand:
                return "Mid Output";
            case OutputPort::LowBand:
                return "Low Band Output";
        }
    }

    return BaseProcessor::getTooltipForPort (portIndex, isInput);
}
