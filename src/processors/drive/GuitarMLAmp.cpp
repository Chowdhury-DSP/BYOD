#include "GuitarMLAmp.h"

namespace
{
const std::map<String, String> guitarMLModels {
    { "BluesJR_FullD_json", "Blues Jr." },
    { "TS9_FullD_json", "TS9" },
};
}

GuitarMLAmp::GuitarMLAmp (UndoManager* um) : BaseProcessor ("GuitarML", createParameterLayout(), um)
{
    gainParam = vts.getRawParameterValue ("gain");
    modelParam = vts.getRawParameterValue ("model");

    for (const auto& modelConfig : guitarMLModels)
    {
        for (auto& model : models)
        {
            model.insert (std::make_pair (modelConfig.first, ModelType {}));

            int modelDataSize;
            auto modelData = BinaryData::getNamedResource (modelConfig.first.getCharPointer(), modelDataSize);
            jassert (modelData != nullptr);
            model.at (modelConfig.first).initialise (modelData, modelDataSize, 48000.0);
        }

        modelTypes.push_back (modelConfig.first);
    }

    uiOptions.backgroundColour = Colours::cornsilk.darker();
    uiOptions.powerColour = Colours::cyan;
    uiOptions.info.description = "An implementation of the neural LSTM guitar amp modeller used by the GuitarML project.";
    uiOptions.info.authors = StringArray { "Keith Bloemer", "Jatin Chowdhury" };
    uiOptions.info.infoLink = "https://guitarml.com";
}

ParamLayout GuitarMLAmp::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, "gain", "Gain", -18.0f, 18.0f, 0.0f);

    StringArray modelNames;
    for (const auto& modelConfig : guitarMLModels)
        modelNames.add (modelConfig.second);

    params.push_back (std::make_unique<AudioParameterChoice> ("model",
                                                              "Model",
                                                              modelNames,
                                                              0));

    return { params.begin(), params.end() };
}

void GuitarMLAmp::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    inGain.prepare (spec);
    inGain.setRampDurationSeconds (0.1);

    for (auto& chModels : models)
    {
        for (auto& model : chModels)
            model.second.prepare (sampleRate, samplesPerBlock);
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 5000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void GuitarMLAmp::processAudio (AudioBuffer<float>& buffer)
{
    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    inGain.setGainDecibels (gainParam->load() - 12.0f);
    inGain.process (context);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto modelType = modelTypes[(int) modelParam->load()];
        auto& model = models[ch].at (modelType);

        auto&& channelBlock = block.getSingleChannelBlock ((size_t) ch);
        model.process<true> (channelBlock);
    }

    dcBlocker.processAudio (buffer);
}
