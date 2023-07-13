#include "FuzzMachine.h"
#include "processors/ParameterHelpers.h"

FuzzMachine::FuzzMachine (UndoManager* um)
    : BaseProcessor ("Fuzz Machine", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    fuzzParam.setParameterHandle (getParameterPointer<chowdsp::PercentParameter*> (vts, "fuzz"));
    loadParameterPointer (modelParam, vts, "model");
    loadParameterPointer (volumeParam, vts, "vol");

    uiOptions.backgroundColour = Colours::red.darker (0.15f);
    uiOptions.powerColour = Colours::silver.brighter (0.1f);
    uiOptions.info.description = "Black box model of a custom-built fuzz pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout FuzzMachine::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "fuzz", "Fuzz", 0.5f);
    emplace_param<chowdsp::EnumChoiceParameter<Model>> (params, "model", "Mode", Model::Mode_2, std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { 'z', '.' } });
    createPercentParameter (params, "vol", "Volume", 0.5f);

    return { params.begin(), params.end() };
}

void FuzzMachine::prepare (double sampleRate, int samplesPerBlock)
{
    const auto osRatio = sampleRate >= 80000.0 ? 1 : 2;

    fuzzParam.setRampLength (0.025);
    fuzzParam.prepare (osRatio * sampleRate, osRatio * samplesPerBlock);

    for (int ch = 0; ch < 2; ++ch)
    {
        if ((int) sampleRate % 44100 == 0)
        {
            model_ff_15[ch].initialise (BinaryData::fuzz_15_88_json, BinaryData::fuzz_15_88_jsonSize, 88200.0);
            model_ff_2[ch].initialise (BinaryData::fuzz_2_88_json, BinaryData::fuzz_2_88_jsonSize, 88200.0);
        }
        else
        {
            model_ff_15[ch].initialise (BinaryData::fuzz_15_json, BinaryData::fuzz_15_jsonSize, 96000.0);
            model_ff_2[ch].initialise (BinaryData::fuzz_2_json, BinaryData::fuzz_2_jsonSize, 96000.0);
        }

        model_ff_15[ch].prepare (osRatio * sampleRate, osRatio * samplesPerBlock);
        model_ff_2[ch].prepare (osRatio * sampleRate, osRatio * samplesPerBlock);
    }

    upsampler.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 }, osRatio);
    downsampler.prepare ({ osRatio * sampleRate, osRatio * (uint32_t) samplesPerBlock, 2 }, osRatio);

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
    const auto osBuffer = upsampler.process (buffer);
    const auto osNumSamples = osBuffer.getNumSamples();
    fuzzParam.process (osNumSamples);

    for (auto [ch, data] : chowdsp::buffer_iters::channels (osBuffer))
    {
        magic_enum::enum_switch (
            [this, ch = ch, data = data, osNumSamples] (auto modelType)
            {
                static constexpr Model model = modelType;
                if constexpr (model == Model::Mode_1z5)
                    model_ff_15[ch].process<true> (data, { fuzzParam.getSmoothedBuffer(), (size_t) osNumSamples });
                else if constexpr (model == Model::Mode_2)
                    model_ff_2[ch].process<true> (data, { fuzzParam.getSmoothedBuffer(), (size_t) osNumSamples });
            },
            modelParam->get());
    }

    downsampler.process (osBuffer, buffer);

    dcBlocker.processBlock (buffer);

    volume.setGainLinear (1.5f * volumeParam->getCurrentValue());
    volume.process (buffer);
}
