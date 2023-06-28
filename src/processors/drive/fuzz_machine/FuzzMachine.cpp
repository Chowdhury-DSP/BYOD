#include "FuzzMachine.h"
#include "processors/ParameterHelpers.h"

FuzzMachine::FuzzMachine (UndoManager* um)
    : BaseProcessor ("Fuzz Machine", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    fuzzParam.setParameterHandle (getParameterPointer<chowdsp::PercentParameter*> (vts, "fuzz"));
    loadParameterPointer (volumeParam, vts, "vol");

    uiOptions.backgroundColour = Colours::red.darker (0.15f);
    uiOptions.powerColour = Colours::silver.brighter (0.1f);
    uiOptions.info.description = "Fuzz effect based loosely on the \"Fuzz Face\" pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout FuzzMachine::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "fuzz", "Fuzz", 0.5f);
    createPercentParameter (params, "vol", "Volume", 0.5f);

    return { params.begin(), params.end() };
}

void FuzzMachine::prepare (double sampleRate, int samplesPerBlock)
{
    model.prepare (sampleRate);

    fuzzParam.setRampLength (0.025);
    fuzzParam.prepare (sampleRate, samplesPerBlock);

    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 2 };
    dcBlocker.prepare (spec);
    dcBlocker.calcCoefs (30.0f, (float) sampleRate);

    volume.setGainLinear (volumeParam->getCurrentValue());
    volume.setRampDurationSeconds (0.05);
    volume.prepare (spec);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    float level = 100.0f;
    while (level > 1.0e-4f)
    {
        buffer.clear();
        processAudio (buffer);
        level = buffer.getMagnitude (0, samplesPerBlock);
    }
}

void FuzzMachine::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    fuzzParam.process (numSamples);

    for (auto [ch, n, data] : chowdsp::buffer_iters::sub_blocks<32, true> (buffer))
    {
        if (ch == 0)
            model.set_gain (fuzzParam.getSmoothedBuffer()[n]);
        model.process (data, ch);
    }

    if (! chowdsp::BufferMath::sanitizeBuffer (buffer))
        model.reset();

    dcBlocker.processBlock (buffer);

    volume.setGainLinear (volumeParam->getCurrentValue());
    volume.process (buffer);
}
