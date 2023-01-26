#include "GuitarMLAmp.h"
#include "gui/utils/ErrorMessageView.h"
#include "gui/utils/ModulatableSlider.h"

namespace
{
const juce::StringArray guitarMLModelResources {
    "BluesJrAmp_VolKnob_json",
    "TS9_DriveKnob_json",
    "MesaRecMini_ModernChannel_GainKnob_json",
};

const juce::StringArray guitarMLModelNames {
    "Blues Jr.",
    "TS9",
    "Mesa Mini Rectifier (Modern)",
};

const auto numBuiltInModels = (int) guitarMLModelResources.size();

const String modelTag = "model";
const String gainTag = "gain";
const String conditionTag = "condition";
const String sampleRateCorrFilterTag = "sample_rate_corr_filter";
const String customModelTag = "custom_model";
constexpr std::string_view modelNameTag = "byod_guitarml_model_name";

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
    using namespace ParameterHelpers;
    loadParameterPointer (gainParam, vts, gainTag);
    conditionParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, conditionTag));
    loadParameterPointer (sampleRateCorrectionFilterParam, vts, sampleRateCorrFilterTag);
    addPopupMenuParameter (sampleRateCorrFilterTag);

    loadModel (0); // load Blues Jr. model by default

    uiOptions.backgroundColour = Colours::cornsilk.darker();
    uiOptions.powerColour = Colours::cyan;
    uiOptions.info.description = "An implementation of the neural LSTM guitar amp modeller used by the GuitarML project. Supports loading custom models that are compatible with the GuitarML Protues plugin";
    uiOptions.info.authors = StringArray { "Keith Bloemer", "Jatin Chowdhury" };
    uiOptions.info.infoLink = "https://guitarml.com";
}

GuitarMLAmp::~GuitarMLAmp() = default;

ParamLayout GuitarMLAmp::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, gainTag, "Gain", -18.0f, 18.0f, 0.0f);
    createPercentParameter (params, conditionTag, "Condition", 0.5f);
    emplace_param<chowdsp::BoolParameter> (params, sampleRateCorrFilterTag, "Sample Rate Correction Filter", true);

    return { params.begin(), params.end() };
}

void GuitarMLAmp::loadModelFromJson (const chowdsp::json& modelJson, const String& newModelName)
{
    const auto setModelWeights = [] (const chowdsp::json& weights_json, auto& model, int hiddenSize)
    {
        auto& lstm = model.template get<0>();
        auto& dense = model.template get<1>();

        Vec2d lstm_weights_ih = weights_json.at ("/state_dict/rec.weight_ih_l0"_json_pointer);
        lstm.setWVals (transpose (lstm_weights_ih));

        Vec2d lstm_weights_hh = weights_json.at ("/state_dict/rec.weight_hh_l0"_json_pointer);
        lstm.setUVals (transpose (lstm_weights_hh));

        std::vector<float> lstm_bias_ih = weights_json.at ("/state_dict/rec.bias_ih_l0"_json_pointer);
        std::vector<float> lstm_bias_hh = weights_json.at ("/state_dict/rec.bias_hh_l0"_json_pointer);
        for (int i = 0; i < 4 * hiddenSize; ++i)
            lstm_bias_hh[(size_t) i] += lstm_bias_ih[(size_t) i];
        lstm.setBVals (lstm_bias_hh);

        Vec2d dense_weights = weights_json.at ("/state_dict/lin.weight"_json_pointer);
        dense.setWeights (dense_weights);

        std::vector<float> dense_bias = weights_json.at ("/state_dict/lin.bias"_json_pointer);
        dense.setBias (dense_bias.data());
    };

    const auto& modelDataJson = modelJson.at ("model_data");
    const auto numInputs = modelDataJson.value ("input_size", 1);
    const auto hiddenSize = modelDataJson.value ("hidden_size", 0);
    const auto modelSampleRate = modelDataJson.value ("sample_rate", 44100.0);
    const auto rnnDelaySamples = jmax (1.0, processSampleRate / modelSampleRate);

    sampleRateCorrectionFilter.reset();
    sampleRateCorrectionFilter.calcCoefsDB (8100.0f,
                                            chowdsp::CoefficientCalculators::butterworthQ<float>,
                                            (processSampleRate < modelSampleRate * 1.1)
                                                ? 0.0f
                                                : -(18.0f + float (processSampleRate / modelSampleRate)),
                                            (float) processSampleRate);

    if (numInputs == 1 && hiddenSize == 40) // non-conditioned LSMT40
    {
        SpinLock::ScopedLockType modelChangingLock { modelChangingMutex };
        for (auto& model : lstm40NoCondModels)
            setModelWeights (modelJson, model, hiddenSize);

        for (auto& model : lstm40NoCondModels)
            model.get<0>().prepare ((float) rnnDelaySamples);

        modelArch = ModelArch::LSTM40NoCond;
    }
    else if (numInputs == 2 && hiddenSize == 40) // conditioned LSMT40
    {
        SpinLock::ScopedLockType modelChangingLock { modelChangingMutex };
        for (auto& model : lstm40CondModels)
            setModelWeights (modelJson, model, hiddenSize);

        for (auto& model : lstm40CondModels)
            model.get<0>().prepare ((float) rnnDelaySamples);

        modelArch = ModelArch::LSTM40Cond;
        conditionParam.reset();
    }
    else
    {
        // unsupported number of inputs!
        throw std::exception();
    }

    cachedModel = modelJson;
    if (newModelName.isNotEmpty())
        cachedModel[modelNameTag] = newModelName;

    modelChangeBroadcaster();
}

void GuitarMLAmp::loadModel (int modelIndex, Component* parentComponent)
{
    normalizationGain = 1.0f;

    if (juce::isPositiveAndBelow (modelIndex, numBuiltInModels))
    {
        int modelDataSize = 0;
        const auto* modelData = BinaryData::getNamedResource (guitarMLModelResources[modelIndex].toRawUTF8(), modelDataSize);
        jassert (modelData != nullptr);

        const auto modelJson = chowdsp::JSONUtils::fromBinaryData (modelData, modelDataSize);
        loadModelFromJson (modelJson, guitarMLModelNames[modelIndex]);

        // The Mesa model is a bit loud, so let's normalize the level down a bit
        // Eventually it would be good to do this sort of thing programmatically.
        // so that it could work for custom loaded models as well.
        if (modelIndex == 2)
            normalizationGain = 0.5f;
    }
    else if (modelIndex == numBuiltInModels)
    {
        customModelChooser = std::make_shared<FileChooser> ("GuitarML Model", File {}, "*.json", true, false, parentComponent);
        customModelChooser->launchAsync (FileBrowserComponent::FileChooserFlags::canSelectFiles,
                                         [this, safeParent = Component::SafePointer { parentComponent }] (const FileChooser& modelChooser)
                                         {
#if JUCE_IOS
                                             const auto chosenFile = modelChooser.getURLResult();
                                             if (chosenFile == URL {})
                                             {
                                                 modelChangeBroadcaster();
                                                 return;
                                             }

                                             try
                                             {
                                                 auto chosenFileStream = chosenFile.createInputStream (URL::InputStreamOptions (URL::ParameterHandling::inAddress));
                                                 const auto& modelJson = chowdsp::JSONUtils::fromInputStream (*chosenFileStream);
                                                 loadModelFromJson (modelJson, chosenFile.getLocalFile().getFileNameWithoutExtension());
                                             }
#else
                const auto chosenFile = modelChooser.getResult();
                if (chosenFile == File {})
                {
                    modelChangeBroadcaster();
                    return;
                }

                try
                {
                    const auto& modelJson = chowdsp::JSONUtils::fromFile (chosenFile);
                    loadModelFromJson (modelJson, chosenFile.getFileNameWithoutExtension());
                }
#endif
                                             catch (const std::exception& exc)
                                             {
                                                 loadModel (0);
                                                 const auto errorMessage = String { "Unable to load GuitarML model from file!\n\n" } + exc.what();
                                                 ErrorMessageView::showErrorMessage ("GuitarML Error",
                                                                                     errorMessage,
                                                                                     "OK",
                                                                                     safeParent.getComponent());
                                             }
                                         });
    }
    else
    {
        // Incorrect model index!
        jassertfalse;
        return;
    }
}

String GuitarMLAmp::getCurrentModelName() const
{
    return cachedModel.value (modelNameTag, "");
}

void GuitarMLAmp::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    inGain.prepare (spec);
    inGain.setRampDurationSeconds (0.1);

    sampleRateCorrectionFilter.prepare (2);

    conditionParam.prepare (sampleRate, samplesPerBlock);
    conditionParam.setRampLength (0.05);

    for (auto& model : lstm40NoCondModels)
        model.get<0>().prepare (1.0f);

    processSampleRate = sampleRate;
    loadModelFromJson (cachedModel);

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

    if (modelArch == ModelArch::LSTM40NoCond)
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
    else if (modelArch == ModelArch::LSTM40Cond)
    {
        conditionParam.process (numSamples);
        const auto* conditionData = conditionParam.getSmoothedBuffer();

        alignas (RTNEURAL_DEFAULT_ALIGNMENT) float inputVec[4] {};
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < numSamples; ++n)
            {
                inputVec[0] = x[n];
                inputVec[1] = conditionData[n];
                x[n] += lstm40CondModels[ch].forward (inputVec);
            }
        }
    }

    if (sampleRateCorrectionFilterParam->get())
    {
        sampleRateCorrectionFilter.processBlock (buffer);
    }

    buffer.applyGain (normalizationGain);

    dcBlocker.processAudio (buffer);
}

std::unique_ptr<XmlElement> GuitarMLAmp::toXML()
{
    auto xml = BaseProcessor::toXML();
    xml->setAttribute (customModelTag, cachedModel.dump());

    return std::move (xml);
}

void GuitarMLAmp::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    const auto modelJsonString = xml->getStringAttribute (customModelTag, {});
    try
    {
        const auto& modelJson = chowdsp::json::parse (modelJsonString.toStdString());
        loadModelFromJson (modelJson);
    }
    catch (...)
    {
        loadModel (0); // go back to Blues Jr. model
    }

    BaseProcessor::fromXML (xml, version, loadPosition);

    if (version < chowdsp::Version { "1.1.4" })
    {
        // Sample rate correction filters were only added in version 1.1.4
        sampleRateCorrectionFilterParam->setValueNotifyingHost (0.0f);
    }
}

bool GuitarMLAmp::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;
    class MainParamSlider : public Slider
    {
    public:
        MainParamSlider (const ModelArch& modelArch,
                         AudioProcessorValueTreeState& vts,
                         ModelChangeBroadcaster& modelChangeBroadcaster,
                         chowdsp::HostContextProvider& hcp)
            : currentModelArch (modelArch),
              gainSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, gainTag), hcp),
              conditionSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, conditionTag), hcp),
              gainAttach (vts, gainTag, gainSlider),
              conditionAttach (vts, conditionTag, conditionSlider)
        {
            for (auto* s : { &gainSlider, &conditionSlider })
                addChildComponent (s);

            hcp.registerParameterComponent (gainSlider, gainSlider.getParameter());
            hcp.registerParameterComponent (conditionSlider, conditionSlider.getParameter());

            modelChangeCallback = modelChangeBroadcaster.connect<&MainParamSlider::updateSliderVisibility> (this);

            this->setName (conditionTag + "__" + gainTag + "__");
        }

        void colourChanged() override
        {
            for (auto* s : { &gainSlider, &conditionSlider })
            {
                for (auto colourID : { Slider::textBoxOutlineColourId,
                                       Slider::textBoxTextColourId,
                                       Slider::textBoxBackgroundColourId,
                                       Slider::textBoxHighlightColourId,
                                       Slider::thumbColourId })
                {
                    s->setColour (colourID, findColour (colourID, false));
                }
            }
        }

        void updateSliderVisibility()
        {
            const auto usingConditionedModel = currentModelArch == ModelArch::LSTM40Cond;

            conditionSlider.setVisible (usingConditionedModel);
            gainSlider.setVisible (! usingConditionedModel);

            setName (usingConditionedModel ? "Condition" : "Gain");
        }

        void visibilityChanged() override
        {
            updateSliderVisibility();
        }

        void resized() override
        {
            for (auto* s : { &gainSlider, &conditionSlider })
            {
                s->setSliderStyle (getSliderStyle());
                s->setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());
            }

            conditionSlider.setBounds (getLocalBounds());
            gainSlider.setBounds (getLocalBounds());
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

        const ModelArch& currentModelArch;
        ModulatableSlider gainSlider, conditionSlider;
        SliderAttachment gainAttach, conditionAttach;

        chowdsp::ScopedCallback modelChangeCallback;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainParamSlider)
    };

    class ModelChoiceBox : public ComboBox
    {
    public:
        ModelChoiceBox (GuitarMLAmp& processor, ModelChangeBroadcaster& modelChangeBroadcaster)
        {
            addItemList (guitarMLModelNames, 1);
            addSeparator();
            addItem ("Custom", guitarMLModelNames.size() + 1);
            setText (processor.getCurrentModelName(), dontSendNotification);

            modelChangeCallback = modelChangeBroadcaster.connect ([this, &processor]
                                                                  { setText (processor.getCurrentModelName(), dontSendNotification); });

            onChange = [this, &processor]
            {
                processor.loadModel (getSelectedItemIndex(), getParentComponent());
            };

            this->setName (modelTag + "__box");
        }

        void visibilityChanged() override
        {
            setName ("Model");
        }

    private:
        chowdsp::ScopedCallback modelChangeCallback;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModelChoiceBox)
    };

    customComps.add (std::make_unique<MainParamSlider> (modelArch, vts, modelChangeBroadcaster, hcp));
    customComps.add (std::make_unique<ModelChoiceBox> (*this, modelChangeBroadcaster));

    return false;
}

void GuitarMLAmp::addToPopupMenu (PopupMenu& menu)
{
    BaseProcessor::addToPopupMenu (menu);
    menu.addItem ("Download more models", []
                  { URL { "https://guitarml.com/tonelibrary/tonelib-pro.html" }.launchInDefaultBrowser(); });
    menu.addSeparator();
}
