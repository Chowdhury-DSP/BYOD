#include "FuzzMachine.h"
#include "processors/ParameterHelpers.h"

namespace
{
constexpr int osRatio = 2;
}

FuzzMachine::FuzzMachine (UndoManager* um)
    : BaseProcessor ("Fuzz Machine", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    fuzzParam.setParameterHandle (getParameterPointer<chowdsp::PercentParameter*> (vts, "fuzz"));
    biasParam.setParameterHandle (getParameterPointer<chowdsp::PercentParameter*> (vts, "bias"));
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
    createPercentParameter (params, "bias", "Bias", 1.0f);

    return { params.begin(), params.end() };
}

void FuzzMachine::prepare (double sampleRate, int samplesPerBlock)
{
    fuzzParam.mappingFunction = [] (float x)
    { return 0.9f * x + 0.05f; };
    fuzzParam.setRampLength (0.025);
    fuzzParam.prepare (sampleRate, samplesPerBlock);

    biasParam.mappingFunction = [] (float x)
    { return 6.5f + 2.5f * x; };
    biasParam.setRampLength (0.025);
    biasParam.prepare (sampleRate, samplesPerBlock);

    upsampler.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 }, osRatio);
    downsampler.prepare ({ osRatio * sampleRate, osRatio * (uint32_t) samplesPerBlock, 2 }, osRatio);

    model_ndk.reset (sampleRate * osRatio);
    model_ndk.Vcc = (double) biasParam.getCurrentValue();
    model_ndk.update_pots ({ FuzzFaceNDK::VRfuzz * (1.0 - (double) fuzzParam.getCurrentValue()),
                             FuzzFaceNDK::VRfuzz * (double) fuzzParam.getCurrentValue() });

    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 2 };
    dcBlocker.prepare (spec);
    dcBlocker.calcCoefs (30.0f, (float) sampleRate);

    volume.setGainLinear (volumeParam->getCurrentValue());
    volume.setRampDurationSeconds (0.05);
    volume.prepare (spec);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    float level = 100.0f;
    int count = 0;
    while (level > 1.0e-4f || count < 100)
    {
        buffer.clear();
        processAudio (buffer);
        level = buffer.getMagnitude (0, samplesPerBlock);
        count++;
    }
}

void FuzzMachine::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    fuzzParam.process (numSamples);
    biasParam.process (numSamples);

    for (auto [ch, data] : chowdsp::buffer_iters::channels (buffer))
    {
        for (auto& sample : data)
            sample = chowdsp::Math::algebraicSigmoid (sample);
    }

    chowdsp::BufferMath::applyGain (buffer, Decibels::decibelsToGain (-72.0f));

    const auto osBuffer = upsampler.process (buffer);
    if (fuzzParam.isSmoothing() || biasParam.isSmoothing())
    {
        for (auto [ch, n, data] : chowdsp::buffer_iters::sub_blocks<32, true> (osBuffer))
        {
            if (ch == 0)
            {
                model_ndk.Vcc = (double) biasParam.getSmoothedBuffer()[n / osRatio];
                const auto fuzzAmt = fuzzParam.getSmoothedBuffer()[n / osRatio];
                model_ndk.update_pots ({ FuzzFaceNDK::VRfuzz * (1.0 - (double) fuzzAmt),
                                         FuzzFaceNDK::VRfuzz * (double) fuzzAmt });
            }
            model_ndk.process (data, ch);
        }
    }
    else
    {
        for (auto [ch, data] : chowdsp::buffer_iters::channels (osBuffer))
            model_ndk.process (data, ch);
    }
    downsampler.process (osBuffer, buffer);

    if (! chowdsp::BufferMath::sanitizeBuffer (buffer))
        model_ndk.reset_state();

    dcBlocker.processBlock (buffer);

    volume.setGainLinear (std::pow (volumeParam->getCurrentValue(), 2.0f) * 40.0f);
    volume.process (buffer);
}
