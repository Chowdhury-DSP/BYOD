#include "GuitarMLAmp.h"

namespace
{
const juce::StringArray guitarMLModelResources {
    "BluesJrAmp_VolKnob_json",
    "TS9_DriveKnob_json",
};

const juce::StringArray guitarMLModelNames {
    "Blues Jr.",
    "TS9",
    "Custom",
};

const auto numBuiltInModels = (int) guitarMLModelResources.size();

const String modelTag = "model";
const String gainTag = "gain";
const String customModelTag = "custom_model";

using Vec2d = std::vector<std::vector<float>>;
auto transpose = [] (const Vec2d& x) -> Vec2d
{
    auto outer_size = x.size();
    auto inner_size = x[0].size();
    Vec2d y (inner_size, std::vector<float> (outer_size, 0.0f));

    for (size_t i = 0; i < outer_size; ++i)
    {
        for (size_t j = 0; j < inner_size; ++j)
            y[j][i] = x[i][j];
    }

    return std::move (y);
};
} // namespace

GuitarMLAmp::GuitarMLAmp (UndoManager* um) : BaseProcessor ("GuitarML", createParameterLayout(), um)
{
    chowdsp::ParamUtils::loadParameterPointer (gainParam, vts, gainTag);
    modelParam = vts.getRawParameterValue (modelTag);

    vts.addParameterListener (modelTag, this);

    uiOptions.backgroundColour = Colours::cornsilk.darker();
    uiOptions.powerColour = Colours::cyan;
    uiOptions.info.description = "An implementation of the neural LSTM guitar amp modeller used by the GuitarML project.";
    uiOptions.info.authors = StringArray { "Keith Bloemer", "Jatin Chowdhury" };
    uiOptions.info.infoLink = "https://guitarml.com";
}

GuitarMLAmp::~GuitarMLAmp()
{
    vts.removeParameterListener (modelTag, this);
}

ParamLayout GuitarMLAmp::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, gainTag, "Gain", -18.0f, 18.0f, 0.0f);

    params.push_back (std::make_unique<AudioParameterChoice> (modelTag,
                                                              "Model",
                                                              guitarMLModelNames,
                                                              0));

    return { params.begin(), params.end() };
}

void GuitarMLAmp::loadModelFromJson (const chowdsp::json& modelJson)
{
    const auto setModelWeights = [] (const chowdsp::json& weights_json, auto& model, int hiddenSize)
    {
        auto& lstm = model.template get<0>();
        auto& dense = model.template get<1>();

        Vec2d lstm_weights_ih = weights_json["/state_dict/rec.weight_ih_l0"_json_pointer];
        lstm.setWVals (transpose (lstm_weights_ih));

        Vec2d lstm_weights_hh = weights_json["/state_dict/rec.weight_hh_l0"_json_pointer];
        lstm.setUVals (transpose (lstm_weights_hh));

        std::vector<float> lstm_bias_ih = weights_json["/state_dict/rec.bias_ih_l0"_json_pointer];
        std::vector<float> lstm_bias_hh = weights_json["/state_dict/rec.bias_hh_l0"_json_pointer];
        for (int i = 0; i < 4 * hiddenSize; ++i)
            lstm_bias_hh[(size_t) i] += lstm_bias_ih[(size_t) i];
        lstm.setBVals (lstm_bias_hh);

        Vec2d dense_weights = weights_json["/state_dict/lin.weight"_json_pointer];
        dense.setWeights (dense_weights);

        std::vector<float> dense_bias = weights_json["/state_dict/lin.bias"_json_pointer];
        dense.setBias (dense_bias.data());
    };

    if (! modelJson.contains ("model_data"))
        return;

    const auto& modelDataJson = modelJson["model_data"];
    const auto numInputs = modelDataJson.value ("num_inputs", 1);
    const auto hiddenSize = modelDataJson.value ("hidden_size", 0);
    const auto modelSampleRate = modelDataJson.value ("sample_rate", 44100.0); // @TODO: sync with Keith for the correct JSON key
    const auto rnnDelaySamples = jmax (1.0, processSampleRate / modelSampleRate);

    if (numInputs == 1 && hiddenSize == 40) // non-conditioned LSMT40
    {
        SpinLock::ScopedLockType modelChangingLock { modelChangingMutex };
        for (auto& model : lstm40NoCondModels)
            setModelWeights (modelJson, model, hiddenSize);

        for (auto& model : lstm40NoCondModels)
            model.get<0>().prepare ((float) rnnDelaySamples);

        modelConfig = ModelConfig { ModelConfig::LSTM40NoCond, modelSampleRate };
    }
    else if (numInputs == 2 && hiddenSize == 40) // conditioned LSMT40
    {
        SpinLock::ScopedLockType modelChangingLock { modelChangingMutex };
        for (auto& model : lstm40CondModels)
            setModelWeights (modelJson, model, hiddenSize);

        for (auto& model : lstm40CondModels)
            model.get<0>().prepare ((float) rnnDelaySamples);

        modelConfig = ModelConfig { ModelConfig::LSTM40Cond, modelSampleRate };
    }
    else
    {
        // unsupported number of inputs!
        jassertfalse;
    }
}

void GuitarMLAmp::loadModel (int modelIndex)
{
    if (juce::isPositiveAndBelow (modelIndex, numBuiltInModels))
    {
        int modelDataSize = 0;
        const auto* modelData = BinaryData::getNamedResource (guitarMLModelResources[modelIndex].toRawUTF8(), modelDataSize);
        jassert (modelData != nullptr);

        const auto modelJson = chowdsp::JSONUtils::fromBinaryData (modelData, modelDataSize);
        loadModelFromJson (modelJson);
    }
    else if (modelIndex == numBuiltInModels)
    {
        if (loadCachedCustomModel)
        {
            if (cachedCustomModel.empty())
            {
                // trying to load a cached custom model, but none had been set!
                jassertfalse;
                return;
            }

            loadModelFromJson (cachedCustomModel);
        }
        else
        {
            customModelChooser = std::make_shared<FileChooser> ("GuitarML Model", File {}, "*.json");
            customModelChooser->launchAsync (FileBrowserComponent::FileChooserFlags::canSelectFiles,
                                             [this] (const FileChooser& modelChooser)
                                             {
                                                 const auto chosenFile = modelChooser.getResult();
                                                 if (chosenFile == File {})
                                                     return;

                                                 try
                                                 {
                                                     const auto& modelJson = chowdsp::JSONUtils::fromFile (chosenFile);
                                                     loadModelFromJson (modelJson);
                                                     cachedCustomModel = modelJson;
                                                 }
                                                 catch (...)
                                                 {
                                                     NativeMessageBox::showAsync (MessageBoxOptions()
                                                                                      .withButton ("Ok")
                                                                                      .withIconType (MessageBoxIconType::WarningIcon)
                                                                                      .withTitle ("GuitarML Error")
                                                                                      .withMessage ("Unable to load GuitarML model from file!"),
                                                                                  nullptr);
                                                 }
                                             });
        }
    }
    else
    {
        // Incorrect model index!
        jassertfalse;
        return;
    }
}

void GuitarMLAmp::parameterChanged (const String& paramID, float newValue)
{
    if (paramID != modelTag)
    {
        // Why are we listening to this parameter??
        jassertfalse;
        return;
    }

    loadModel ((int) newValue);
}

void GuitarMLAmp::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    inGain.prepare (spec);
    inGain.setRampDurationSeconds (0.1);

    for (auto& model : lstm40NoCondModels)
        model.get<0>().prepare (1.0f);

    processSampleRate = sampleRate;
    {
        ScopedValueSetter scopedLoadCachedCustomModel { loadCachedCustomModel, true };
        loadModel ((int) modelParam->load());
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
    const SpinLock::ScopedTryLockType modelChangingLock { modelChangingMutex };
    if (! modelChangingLock.isLocked())
        return;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (modelConfig.modelType == ModelConfig::LSTM40NoCond)
    {
        inGain.setGainDecibels (gainParam->getCurrentValue() - 12.0f);
        inGain.process (buffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < numSamples; ++n)
                x[n] += lstm40NoCondModels[ch].forward (&x[n]);
        }
    }
    else if (modelConfig.modelType == ModelConfig::LSTM40Cond)
    {
        alignas (RTNEURAL_DEFAULT_ALIGNMENT) float inputVec[4] {};
        inputVec[1] = gainParam->getCurrentValue() / 36.0f + 0.5f; // @TODO: Some parameter smoothing?
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < numSamples; ++n)
            {
                inputVec[0] = x[n];
                x[n] += lstm40CondModels[ch].forward (inputVec);
            }
        }
    }

    dcBlocker.processAudio (buffer);
}

std::unique_ptr<XmlElement> GuitarMLAmp::toXML()
{
    auto xml = BaseProcessor::toXML();
    xml->setAttribute (customModelTag, cachedCustomModel.dump());

    return std::move (xml);
}

void GuitarMLAmp::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    const auto cachedModelJsonString = xml->getStringAttribute (customModelTag, {});
    try
    {
        cachedCustomModel = chowdsp::json::parse (cachedModelJsonString.toStdString());
    }
    catch (...)
    {
        cachedCustomModel = {};
    }

    ScopedValueSetter scopedLoadCachedCustomModel { loadCachedCustomModel, true };
    BaseProcessor::fromXML (xml, version, loadPosition);
}
