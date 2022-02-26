#include "GuitarMLAmp.h"

namespace
{
const std::map<String, String> guitarMLModels {
    { "BluesJR_FullD_json", "Blues Jr." },
    { "TS9_FullD_json", "TS9" },
};

using Vec2d = std::vector<std::vector<float>>;

Vec2d transpose (const Vec2d& x)
{
    auto outer_size = x.size();
    auto inner_size = x[0].size();
    Vec2d y (inner_size, std::vector<float> (outer_size, 0.0f));

    for (size_t i = 0; i < outer_size; ++i)
    {
        for (size_t j = 0; j < inner_size; ++j)
            y[j][i] = x[i][j];
    }

    return y;
}

void load_json (const String& filename, GuitarMLAmp::ModelType& model)
{
    auto& lstm = model.get<0>();
    auto& dense = model.get<1>();

    // read a JSON file
    int modelDataSize;
    auto modelData = BinaryData::getNamedResource (filename.getCharPointer(), modelDataSize);
    jassert (modelData != nullptr);
    MemoryInputStream jsonInputStream (modelData, (size_t) modelDataSize, false);
    auto weights_json = nlohmann::json::parse (jsonInputStream.readEntireStreamAsString().toStdString());

    Vec2d lstm_weights_ih = weights_json["/state_dict/rec.weight_ih_l0"_json_pointer];
    lstm.setWVals (transpose (lstm_weights_ih));

    Vec2d lstm_weights_hh = weights_json["/state_dict/rec.weight_hh_l0"_json_pointer];
    lstm.setUVals (transpose (lstm_weights_hh));

    std::vector<float> lstm_bias_ih = weights_json["/state_dict/rec.bias_ih_l0"_json_pointer];
    std::vector<float> lstm_bias_hh = weights_json["/state_dict/rec.bias_hh_l0"_json_pointer];
    for (int i = 0; i < 80; ++i)
        lstm_bias_hh[i] += lstm_bias_ih[i];
    lstm.setBVals (lstm_bias_hh);

    Vec2d dense_weights = weights_json["/state_dict/lin.weight"_json_pointer];
    dense.setWeights (dense_weights);

    std::vector<float> dense_bias = weights_json["/state_dict/lin.bias"_json_pointer];
    dense.setBias (dense_bias.data());
}

} // namespace

GuitarMLAmp::GuitarMLAmp (UndoManager* um) : BaseProcessor ("GuitarML", createParameterLayout(), um)
{
    gainParam = vts.getRawParameterValue ("gain");
    modelParam = vts.getRawParameterValue ("model");

    for (const auto& modelConfig : guitarMLModels)
    {
        for (auto& model : models)
        {
            model.insert (std::make_pair (modelConfig.first, ModelType {}));
            load_json (modelConfig.first, model.at (modelConfig.first));
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
    inGain.setRampDurationSeconds (0.01);

    for (auto& chModels : models)
    {
        for (auto& model : chModels)
            model.second.reset();
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
        auto* x = buffer.getWritePointer (ch);

        auto modelType = modelTypes[(int) modelParam->load()];
        auto& model = models[ch].at (modelType);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
            x[n] = model.forward (x + n) + x[n];
    }

    dcBlocker.processAudio (buffer);
}
