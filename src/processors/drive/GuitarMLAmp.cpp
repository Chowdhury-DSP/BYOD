#include "GuitarMLAmp.h"
#include "gui/utils/ErrorMessageView.h"
#include "gui/utils/ModulatableSlider.h"

namespace RONNTags
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
} // namespace RONNTags

GuitarMLAmp::GuitarMLAmp (UndoManager* um) : BaseProcessor ("GuitarML", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (gainParam, vts, RONNTags::gainTag);
    conditionParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, RONNTags::conditionTag));
    loadParameterPointer (sampleRateCorrectionFilterParam, vts, RONNTags::sampleRateCorrFilterTag);
    addPopupMenuParameter (RONNTags::sampleRateCorrFilterTag);

    loadModel (0); // load Blues Jr. model by default

    uiOptions.backgroundColour = Colours::cornsilk.darker();
    uiOptions.powerColour = Colours::cyan;
    uiOptions.info.description = "An implementation of the neural LSTM guitar amp modeller used by the GuitarML project. Supports loading custom models that are compatible with the GuitarML Protues plugin";
    uiOptions.info.authors = StringArray { "Keith Bloemer", "Jatin Chowdhury" };
    uiOptions.info.infoLink = "https://guitarml.com";

#if JUCE_INTEL
    if (juce::SystemStats::hasAVX() && juce::SystemStats::hasFMA3())
    {
        juce::Logger::writeToLog ("Using RNN model with AVX SIMD instructions!");
        lstm40CondModels[0].template emplace<rnn_avx::RNNAccelerated<2, 40, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>>();
        lstm40CondModels[1].template emplace<rnn_avx::RNNAccelerated<2, 40, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>>();
        lstm40NoCondModels[0].template emplace<rnn_avx::RNNAccelerated<1, 40, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>>();
        lstm40NoCondModels[1].template emplace<rnn_avx::RNNAccelerated<1, 40, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>>();
    }
#endif
}

GuitarMLAmp::~GuitarMLAmp() = default;

ParamLayout GuitarMLAmp::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, RONNTags::gainTag, "Gain", -18.0f, 18.0f, 0.0f);
    createPercentParameter (params, RONNTags::conditionTag, "Condition", 0.5f);
    emplace_param<chowdsp::BoolParameter> (params, RONNTags::sampleRateCorrFilterTag, "Sample Rate Correction Filter", true);

    return { params.begin(), params.end() };
}

void GuitarMLAmp::loadModelFromJson (const chowdsp::json& modelJson, const String& newModelName)
{
    const auto& modelDataJson = modelJson.at ("model_data");
    const auto numInputs = modelDataJson.value ("input_size", 1);
    const auto hiddenSize = modelDataJson.value ("hidden_size", 0);
    const auto modelSampleRate = modelDataJson.value ("sample_rate", 44100.0);
    const auto rnnDelaySamples = jmax (1.0, processSampleRate / modelSampleRate);

    sampleRateCorrectionFilter.reset();
    sampleRateCorrectionFilter.calcCoefs (8100.0f,
                                          chowdsp::CoefficientCalculators::butterworthQ<float>,
                                          (processSampleRate < modelSampleRate * 1.1) ? 1.0f : 0.25f,
                                          (float) processSampleRate);

    if (numInputs == 1 && hiddenSize == 40) // non-conditioned LSMT40
    {
        SpinLock::ScopedLockType modelChangingLock { modelChangingMutex };
        for (auto& modelVariant : lstm40NoCondModels)
        {
            modelVariant.visit (
                [rnnDelaySamples, &modelJson] (auto& model)
                {
                    model.initialise (modelJson);
                    model.prepare ((float) rnnDelaySamples);
                });
        }

        modelArch = ModelArch::LSTM40NoCond;
    }
    else if (numInputs == 2 && hiddenSize == 40) // conditioned LSMT40
    {
        SpinLock::ScopedLockType modelChangingLock { modelChangingMutex };
        for (auto& modelVariant : lstm40CondModels)
        {
            modelVariant.visit (
                [rnnDelaySamples, &modelJson] (auto& model)
                {
                    model.initialise (modelJson);
                    model.prepare ((float) rnnDelaySamples);
                });
        }

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
        cachedModel[RONNTags::modelNameTag] = newModelName;

    modelChangeBroadcaster();
}

void GuitarMLAmp::loadModel (int modelIndex, Component* parentComponent)
{
    normalizationGain = 1.0f;

    if (juce::isPositiveAndBelow (modelIndex, RONNTags::numBuiltInModels))
    {
        int modelDataSize = 0;
        const auto* modelData = BinaryData::getNamedResource (RONNTags::guitarMLModelResources[modelIndex].toRawUTF8(), modelDataSize);
        jassert (modelData != nullptr);

        const auto modelJson = chowdsp::JSONUtils::fromBinaryData (modelData, modelDataSize);
        loadModelFromJson (modelJson, RONNTags::guitarMLModelNames[modelIndex]);

        // The Mesa model is a bit loud, so let's normalize the level down a bit
        // Eventually it would be good to do this sort of thing programmatically.
        // so that it could work for custom loaded models as well.
        if (modelIndex == 2)
            normalizationGain = 0.5f;
    }
    else if (modelIndex == RONNTags::numBuiltInModels)
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
    return cachedModel.value (RONNTags::modelNameTag, "");
}

void GuitarMLAmp::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    inGain.prepare (spec);
    inGain.setRampDurationSeconds (0.1);

    sampleRateCorrectionFilter.prepare (2);

    conditionParam.prepare (sampleRate, samplesPerBlock);
    conditionParam.setRampLength (0.05);

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
            lstm40NoCondModels[ch].visit ([x, numSamples] (auto& model)
                                          { model.process ({ x, (size_t) numSamples }, true); });
        }
    }
    else if (modelArch == ModelArch::LSTM40Cond)
    {
        conditionParam.process (numSamples);
        const auto* conditionData = conditionParam.getSmoothedBuffer();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            lstm40CondModels[ch].visit ([x, conditionData, numSamples] (auto& model)
                                        { model.process_conditioned ({ x, (size_t) numSamples }, { conditionData, (size_t) numSamples }, true); });
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
    xml->setAttribute (RONNTags::customModelTag, cachedModel.dump());

    return std::move (xml);
}

void GuitarMLAmp::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    const auto modelJsonString = xml->getStringAttribute (RONNTags::customModelTag, {});
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

    using namespace std::string_view_literals;
    if (version < chowdsp::Version { "1.1.4"sv })
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
        MainParamSlider (const ModelArch& modelArchitecture,
                         AudioProcessorValueTreeState& vts,
                         ModelChangeBroadcaster& modelChangeCaster,
                         chowdsp::HostContextProvider& hcp)
            : currentModelArch (modelArchitecture),
              gainSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, RONNTags::gainTag), hcp),
              conditionSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, RONNTags::conditionTag), hcp),
              gainAttach (vts, RONNTags::gainTag, gainSlider),
              conditionAttach (vts, RONNTags::conditionTag, conditionSlider)
        {
            for (auto* s : { &gainSlider, &conditionSlider })
                addChildComponent (s);

            hcp.registerParameterComponent (gainSlider, gainSlider.getParameter());
            hcp.registerParameterComponent (conditionSlider, conditionSlider.getParameter());

            modelChangeCallback = modelChangeCaster.connect<&MainParamSlider::updateSliderVisibility> (this);

            Component::setName (RONNTags::conditionTag + "__" + RONNTags::gainTag + "__");
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
        ModelChoiceBox (GuitarMLAmp& processor, ModelChangeBroadcaster& modelChangeCaster)
        {
            addItemList (RONNTags::guitarMLModelNames, 1);
            addSeparator();
            addItem ("Custom", RONNTags::guitarMLModelNames.size() + 1);
            setText (processor.getCurrentModelName(), dontSendNotification);

            modelChangeCallback = modelChangeCaster.connect ([this, &processor]
                                                             { setText (processor.getCurrentModelName(), dontSendNotification); });

            onChange = [this, &processor]
            {
                processor.loadModel (getSelectedItemIndex(), getTopLevelComponent());
            };

            Component::setName (RONNTags::modelTag + "__box");
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
