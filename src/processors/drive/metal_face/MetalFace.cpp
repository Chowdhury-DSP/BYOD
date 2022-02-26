#include "MetalFace.h"
#include "../../ParameterHelpers.h"

MetalFace::MetalFace (UndoManager* um) : BaseProcessor ("Metal Face", createParameterLayout(), um)
{
    gainDBParam = vts.getRawParameterValue ("gain");

    uiOptions.backgroundColour = Colours::darkred.brighter (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "Emulation of a HEAVY distortion signal chain.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    for (auto& model : rnn)
        model.initialise (BinaryData::metal_face_model_json, BinaryData::metal_face_model_jsonSize, 96000.0);
}

ParamLayout MetalFace::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, "gain", "Gain", -48.0f, 6.0f, -12.0f, -12.0f);

    return { params.begin(), params.end() };
}

void MetalFace::prepare (double sampleRate, int samplesPerBlock)
{
    gain.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    gain.setRampDurationSeconds (0.1);

    for (auto& model : rnn)
        model.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 5000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void MetalFace::processAudio (AudioBuffer<float>& buffer)
{
    auto&& block = dsp::AudioBlock<float> { buffer };

    const auto gainDB = gainDBParam->load();
    gain.setGainDecibels (gainDB);
    gain.process (dsp::ProcessContextReplacing<float> { block });

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto&& channelBlock = block.getSingleChannelBlock ((size_t) ch);
        rnn[ch].process (channelBlock);
    }

    const auto makeupDB = (-48.0f - gainDB) / 10.0f;
    block *= Decibels::decibelsToGain (makeupDB);
}
