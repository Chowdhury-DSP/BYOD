#include "Octaver.h"
#include "processors/BufferHelpers.h"
#include "processors/ParameterHelpers.h"

namespace
{
const String trackingTag = "tracking";
const String cutoffTag = "cutoff";
const String mixTag = "mix";
} // namespace

Octaver::Octaver (UndoManager* um) : BaseProcessor ("Octaver", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (trackingParam, vts, trackingTag);
    loadParameterPointer (cutoffParam, vts, cutoffTag);
    loadParameterPointer (mixParam, vts, mixTag);

    uiOptions.backgroundColour = Colour { 0xff5c96ac };
    uiOptions.powerColour = Colour { 0xffd8d737 };
    uiOptions.info.description = "A smooth sub-octave effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Octaver::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createFreqParameter (params, trackingTag, "Tracking", 300.0f, 1000.0f, 650.0f, 650.0f);
    createFreqParameter (params, cutoffTag, "Cutoff", 500.0f, 2000.0f, 1000.0f, 2000.0f);
    createPercentParameter (params, mixTag, "Mix", 0.75f);

    return { params.begin(), params.end() };
}

void Octaver::prepare (double sampleRate, int samplesPerBlock)
{
    dryBuffer.setSize (2, samplesPerBlock);
    envelopeBuffer.setSize (1, samplesPerBlock);

    const auto spec = dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 2 };

    inputLPF.prepare (spec);
    inputHPF.prepare (spec);
    outputLPF.prepare (spec);

    levelTracker.prepare (spec);
    levelTracker.setParameters (0.1f, 15.0f);

    outputGain.prepare (spec);
    outputGain.setRampDurationSeconds (0.05);

    for (auto& d : divider)
        d.reset();
}

void Octaver::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    dryBuffer.makeCopyOf (buffer, true);

    // process input filters
    const auto trackingFreq = trackingParam->getCurrentValue();
    inputHPF.setCutoffFrequency (trackingFreq * 0.33f);
    inputHPF.processBlock (buffer);
    inputLPF.setCutoffFrequency (trackingFreq);
    inputLPF.processBlock (buffer);
    buffer.applyGain (4.0f);

    // process level tracker
    {
        envelopeBuffer.setSize (numChannels, numSamples, false, false, true);
        BufferHelpers::collapseToMonoBuffer (buffer, envelopeBuffer);

        auto&& envelopeBlock = dsp::AudioBlock<float> { envelopeBuffer };
        auto envelopeContext = dsp::ProcessContextNonReplacing<float> { dsp::AudioBlock<float> { buffer }, envelopeBlock };
        levelTracker.process (envelopeContext);
    }

    // Generate Square-Wave in-phase
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < numSamples; ++n)
        {
            auto Q = (x[n] > -x[n]) && (std::abs (x[n]) > 1.0e-4f);
            Q = divider[ch].process (Q);
            x[n] = Q ? x[n] : -x[n];
        }
    }

    // multiply by envelope
    for (int ch = 0; ch < numChannels; ++ch)
    {
        FloatVectorOperations::multiply (buffer.getWritePointer (ch),
                                         envelopeBuffer.getReadPointer (0),
                                         numSamples);
    }

    // process the output filter
    outputLPF.setCutoffFrequency (cutoffParam->getCurrentValue());
    outputLPF.processBlock (buffer);

    // mix the output signal
    const auto mixAmount = mixParam->getCurrentValue();
    if (mixAmount < 1.0e-6f)
        outputGain.setGainLinear (0.0f);
    else
        outputGain.setGainDecibels (mixAmount * 48.0f - 24.0f);
    outputGain.process (buffer);

    for (int ch = 0; ch < numChannels; ++ch)
        buffer.addFrom (ch, 0, dryBuffer, ch, 0, numSamples);
}
