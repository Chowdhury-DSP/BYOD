#include "NAMProcessor.h"

#if ! JUCE_IOS

#include <NeuralAudio/NeuralModel.h>

#include "gui/utils/ErrorMessageView.h"
#include "gui/utils/ModulatableSlider.h"
#include "processors/ParameterHelpers.h"

namespace NAMTags
{
const String inputGainTag = "nam_input";
const String outputGainTag = "nam_output";
const String qualityTag = "nam_quality";
const String inputCalTag = "nam_input_cal";
const String calibrateTag = "nam_calibrate";
const String modelPathTag = "nam_model_path";
} // namespace NAMTags

NAMProcessor::NAMProcessor (UndoManager* um)
    : BaseProcessor ("NAM", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (inputGainParam, vts, NAMTags::inputGainTag);
    loadParameterPointer (outputGainParam, vts, NAMTags::outputGainTag);
    loadParameterPointer (qualityParam, vts, NAMTags::qualityTag);
    loadParameterPointer (inputCalDBuParam, vts, NAMTags::inputCalTag);
    loadParameterPointer (calibrateParam, vts, NAMTags::calibrateTag);

    uiOptions.backgroundColour = Colour (0xFF2E2E38);
    uiOptions.powerColour = Colour (0xFF19E5C6);
    uiOptions.info.description =
        "Neural Amp Modeler processor: loads .nam and RTNeural JSON models "
        "(e.g. AIDA-X) and runs them in real time via the NeuralAudio engine.";
    uiOptions.info.authors = StringArray {
        "Steven Atkinson (NAM)",
        "Mike Oliphant (NeuralAudio)",
    };
    uiOptions.info.infoLink = "https://github.com/mikeoliphant/NeuralAudio";

    // Calibrate toggle lives inside the model ComboBox popup; skip the
    // automatic knob layout so we don't get a duplicate button.
    uiOptions.paramIDsToSkip.add (NAMTags::calibrateTag);
}

NAMProcessor::~NAMProcessor() = default;

ParamLayout NAMProcessor::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, NAMTags::inputGainTag, "Input", -20.0f, 20.0f, 0.0f);
    createGainDBParameter (params, NAMTags::outputGainTag, "Output", -20.0f, 20.0f, 0.0f);
    createPercentParameter (params, NAMTags::qualityTag, "Quality", 1.0f);

    // Input calibration: user's audio-interface max input level in dBu (the
    // analog level that drives it to 0 dBFS). Combined with the model's
    // captured input_level_dbu to gain-match the digital signal to the
    // voltage the real amp saw during capture.
    static const auto dBuValToString = [] (float v)
    { return juce::String (v, 1) + " dBu"; };
    static const auto stringTodBuVal = [] (const juce::String& s)
    { return s.getFloatValue(); };
    emplace_param<chowdsp::FloatParameter> (
        params,
        NAMTags::inputCalTag,
        "Input Cal",
        juce::NormalisableRange<float> { 0.0f, 30.0f },
        12.0f,
        std::function<juce::String (float)> { dBuValToString },
        std::function<float (const juce::String&)> { stringTodBuVal });

    emplace_param<chowdsp::BoolParameter> (params, NAMTags::calibrateTag, "Calibrate", true);

    return { params.begin(), params.end() };
}

NAMProcessor::ModelLoadResult NAMProcessor::buildModelsForPath (const String& path)
{
    ModelLoadResult result;
    const auto pathStd = path.toStdString();

    for (size_t ch = 0; ch < result.models.size(); ++ch)
    {
        NeuralAudio::NeuralModelLoader loader;
        loader.SetExternalSampleRate ((int) processSampleRate);
        loader.SetDefaultMaxAudioBufferSize (processMaxBlockSize);
        // Use 0 dBu as baseline so we can back-solve modelInputLevelDBu from
        // GetRecommendedInputDBAdjustment() (== 0 - modelInputLevelDBu).
        loader.SetAudioInputLevelDBu (0.0f);

        auto* raw = loader.CreateFromFile (pathStd);
        if (raw == nullptr)
            throw std::runtime_error ("CreateFromFile returned null (unsupported "
                                      "architecture or malformed file)");

        result.models[ch].reset (raw);
        result.models[ch]->SetMaxAudioBufferSize (processMaxBlockSize);
        result.models[ch]->SetQualityScaleFactor (qualityParam != nullptr
                                                      ? qualityParam->getCurrentValue()
                                                      : 1.0f);

        if (ch == 0)
        {
            result.hasCalibration = ! result.models[ch]->GetMetadata ("input_level_dbu").empty();
            result.modelInputLevelDBu = -result.models[ch]->GetRecommendedInputDBAdjustment();
            result.calOutputAdjustDB = result.models[ch]->GetRecommendedOutputDBAdjustment();
        }
    }

    return result;
}

void NAMProcessor::clearModel()
{
    {
        SpinLock::ScopedLockType lock { modelChangingMutex };
        for (auto& m : models)
            m.reset();
        modelsMaxBlockSize = 0;
        cachedModelPath.clear();
        currentModelName.clear();
        modelHasCalibration = false;
        modelInputLevelDBu = 12.0f;
        calOutputAdjustDB = 0.0f;
    }
    modelChangeBroadcaster();
}

static void reportNAMError (const String& title, const String& message, Component* parent)
{
    Logger::writeToLog ("[NAM] " + title + ": " + message);

    // Prefer BYOD's in-plugin ErrorMessageView; fall back to a native async
    // dialog if the parent is detached.
    if (parent != nullptr)
    {
        ErrorMessageView::showErrorMessage (title, message, "OK", parent);
    }
    else
    {
        NativeMessageBox::showAsync (MessageBoxOptions()
                                         .withIconType (MessageBoxIconType::WarningIcon)
                                         .withTitle (title)
                                         .withMessage (message)
                                         .withButton ("OK"),
                                     nullptr);
    }
}

void NAMProcessor::loadModelFromFile (const File& file, Component* parent)
{
    if (file == File {} || ! file.existsAsFile())
    {
        reportNAMError ("NAM Error", "Selected model file does not exist:\n" + file.getFullPathName(), parent);
        return;
    }

    // Two-phase commit: build the full result first, only swap in on success.
    ModelLoadResult loaded;
    try
    {
        loaded = buildModelsForPath (file.getFullPathName());
    }
    catch (const std::exception& exc)
    {
        reportNAMError ("NAM Error",
                        String { "Unable to load NAM model:\n\n" } + exc.what()
                            + "\n\nFile: " + file.getFullPathName(),
                        parent);
        return;
    }
    catch (...)
    {
        reportNAMError ("NAM Error",
                        "Unknown error while loading NAM model.\n\nFile: " + file.getFullPathName(),
                        parent);
        return;
    }

    {
        SpinLock::ScopedLockType lock { modelChangingMutex };
        for (size_t ch = 0; ch < models.size(); ++ch)
            models[ch] = std::move (loaded.models[ch]);
        modelsMaxBlockSize = processMaxBlockSize;
        cachedModelPath = file.getFullPathName();
        currentModelName = file.getFileNameWithoutExtension();
        modelHasCalibration = loaded.hasCalibration;
        modelInputLevelDBu = loaded.modelInputLevelDBu;
        calOutputAdjustDB = loaded.calOutputAdjustDB;
        lastQualityApplied = qualityParam != nullptr ? qualityParam->getCurrentValue() : 1.0f;
    }

    modelChangeBroadcaster();
}

void NAMProcessor::chooseModel (Component* parent)
{
    // Start the chooser in the directory of the currently loaded model so
    // switching between variants in the same folder is a single click.
    const File startLocation = cachedModelPath.isNotEmpty()
                                   ? File { cachedModelPath }.getParentDirectory()
                                   : File {};

    modelChooser = std::make_shared<FileChooser> ("Load NAM Model",
                                                  startLocation,
                                                  "*.nam;*.json",
                                                  true,
                                                  false,
                                                  parent);

    modelChooser->launchAsync (
        FileBrowserComponent::FileChooserFlags::canSelectFiles,
        [this, safeParent = Component::SafePointer { parent }] (const FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file == File {})
                return;
            loadModelFromFile (file, safeParent.getComponent());
        });
}

void NAMProcessor::prepare (double sampleRate, int samplesPerBlock)
{
    processSampleRate = sampleRate;
    processMaxBlockSize = samplesPerBlock;

    const auto spec = dsp::ProcessSpec { sampleRate, (uint32) samplesPerBlock, 2 };

    inGain.prepare (spec);
    inGain.setRampDurationSeconds (0.05);
    outGain.prepare (spec);
    outGain.setRampDurationSeconds (0.05);

    dcBlocker.prepare (spec);
    dcBlocker.calcCoefs (20.0f, (float) sampleRate);

    // Reload the current model at the new sample rate / buffer size. WaveNet
    // models bake the sample rate in at load time, so we must recreate.
    // On failure we keep the previous models running rather than dropping to
    // bypass — a stale model is preferable to silent inconsistent state.
    if (cachedModelPath.isNotEmpty())
    {
        try
        {
            auto loaded = buildModelsForPath (cachedModelPath);
            SpinLock::ScopedLockType lock { modelChangingMutex };
            for (size_t ch = 0; ch < models.size(); ++ch)
                models[ch] = std::move (loaded.models[ch]);
            modelsMaxBlockSize = processMaxBlockSize;
            modelHasCalibration = loaded.hasCalibration;
            modelInputLevelDBu = loaded.modelInputLevelDBu;
            calOutputAdjustDB = loaded.calOutputAdjustDB;
            lastQualityApplied = qualityParam != nullptr ? qualityParam->getCurrentValue() : 1.0f;
        }
        catch (...)
        {
            // The stale models stay live (see comment above), but
            // modelsMaxBlockSize deliberately keeps its old value:
            // processAudio() bypasses the models for any block larger than
            // they were built for, and processing resumes automatically
            // once a reload succeeds.
            Logger::writeToLog ("NAM: prepare() model reload failed for " + cachedModelPath);
        }
    }
}

void NAMProcessor::processAudio (AudioBuffer<float>& buffer)
{
    const SpinLock::ScopedTryLockType lock { modelChangingMutex };
    if (! lock.isLocked())
        return; // model is being swapped; pass audio through unchanged this block

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    // Calibration only engages when the toggle is on AND the model carries
    // input_level_dbu metadata. Otherwise the cal offsets are zero.
    const bool applyCalibration = modelHasCalibration && calibrateParam->get();
    const float calInDB = applyCalibration
                              ? (inputCalDBuParam->getCurrentValue() - modelInputLevelDBu)
                              : 0.0f;
    const float calOutDB = applyCalibration ? calOutputAdjustDB : 0.0f;

    // Signal chain: (user-in + cal-in) -> model -> (cal-out + user-out) -> DC block
    inGain.setGainDecibels (inputGainParam->getCurrentValue() + calInDB);
    inGain.process (buffer);

    // Only push the quality parameter into the model when it has actually
    // changed AND the model reports the change is realtime-safe. Otherwise
    // the new value takes effect at the next model reload.
    const float newQuality = qualityParam->getCurrentValue();
    const bool qualityChanged = newQuality != lastQualityApplied;
    bool qualityAppliedThisBlock = true;

    // NeuralAudio models must never be given more samples than the max
    // buffer size they were built with: exceeding it is unchecked in release
    // builds, and per the NeuralAudio README, SetMaxAudioBufferSize() is not
    // realtime-safe so we can't resize here. This can happen if a
    // prepare()-time model reload failed while the block size grew (e.g. the
    // model file was deleted and the oversampling factor was raised); in
    // that case, bypass the models until a reload succeeds.
    if (numSamples <= modelsMaxBlockSize)
    {
        for (int ch = 0; ch < numChannels && ch < (int) models.size(); ++ch)
        {
            auto& model = models[(size_t) ch];
            if (model == nullptr)
                continue;

            if (qualityChanged)
            {
                if (model->IsQualityChangeRealtimeSafe (newQuality))
                    model->SetQualityScaleFactor (newQuality);
                else
                    qualityAppliedThisBlock = false;
            }

            auto* x = buffer.getWritePointer (ch);
            model->Process (x, x, (size_t) numSamples);
        }

        if (qualityChanged && qualityAppliedThisBlock)
            lastQualityApplied = newQuality;
    }

    outGain.setGainDecibels (outputGainParam->getCurrentValue() + calOutDB);
    outGain.process (buffer);

    dcBlocker.processBlock (buffer);
}

std::unique_ptr<XmlElement> NAMProcessor::toXML()
{
    auto xml = BaseProcessor::toXML();
    xml->setAttribute (NAMTags::modelPathTag, cachedModelPath);
    return xml;
}

void NAMProcessor::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);

    const auto savedPath = xml->getStringAttribute (NAMTags::modelPathTag, {});
    if (savedPath.isEmpty())
        return;

    const File file { savedPath };
    bool loadFailed = ! file.existsAsFile();

    if (! loadFailed)
    {
        try
        {
            auto loaded = buildModelsForPath (savedPath);
            SpinLock::ScopedLockType lock { modelChangingMutex };
            for (size_t ch = 0; ch < models.size(); ++ch)
                models[ch] = std::move (loaded.models[ch]);
            modelsMaxBlockSize = processMaxBlockSize;
            cachedModelPath = savedPath;
            currentModelName = file.getFileNameWithoutExtension();
            modelHasCalibration = loaded.hasCalibration;
            modelInputLevelDBu = loaded.modelInputLevelDBu;
            calOutputAdjustDB = loaded.calOutputAdjustDB;
            lastQualityApplied = qualityParam != nullptr ? qualityParam->getCurrentValue() : 1.0f;
        }
        catch (...)
        {
            loadFailed = true;
        }
    }

    if (loadFailed)
    {
        // Session references a model we can't load right now. Log-only and
        // preserve the saved path so save-round-trips are stable.
        Logger::writeToLog ("NAM: could not load saved model: " + savedPath);
        SpinLock::ScopedLockType lock { modelChangingMutex };
        for (auto& m : models)
            m.reset();
        modelsMaxBlockSize = 0;
        cachedModelPath = savedPath;
        currentModelName = "(missing) " + File (savedPath).getFileNameWithoutExtension();
        modelHasCalibration = false;
    }

    modelChangeBroadcaster();
}

bool NAMProcessor::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;

    // BYOD's KnobsComponent only lays out Sliders/ComboBoxes as custom
    // components, so we use a ComboBox and override showPopup() to inject a
    // dynamically-disabled "Calibrate Input" toggle. Mirrors GuitarMLAmp.
    class ModelChoiceBox : public ComboBox
    {
    public:
        ModelChoiceBox (NAMProcessor& proc, ModelChangeBroadcaster& caster)
            : processor (proc)
        {
            setText (getDisplayText(), dontSendNotification);
            refreshTooltip();

            modelChangeCallback = caster.connect ([this]
                                                  {
                                                      setText (getDisplayText(), dontSendNotification);
                                                      refreshTooltip();
                                                      repaint();
                                                  });

            Component::setName ("nam_model__box");
        }

        void showPopup() override
        {
            juce::PopupMenu menu;
            menu.setLookAndFeel (&getLookAndFeel());

            menu.addItem (loadItemId, "Load Model...");

            if (processor.getCurrentModelName().isNotEmpty())
                menu.addItem (clearItemId, "Clear Model");

            menu.addSeparator();

            const bool hasCal = processor.currentModelHasCalibration();
            auto* calParam = processor.getCalibrateParam();

            juce::PopupMenu::Item calItem;
            calItem.itemID = calibrateItemId;
            if (hasCal)
            {
                calItem.text = "Calibrate Input  (model: "
                               + juce::String (processor.getModelInputLevelDBu(), 1) + " dBu)";
                calItem.isEnabled = true;
                calItem.isTicked = (calParam != nullptr && calParam->get());
            }
            else
            {
                calItem.text = "Calibrate Input  (model not calibrated)";
                calItem.isEnabled = false;
                calItem.isTicked = false;
            }
            menu.addItem (calItem);

            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                                [safeThis = Component::SafePointer<ModelChoiceBox> { this }] (int result)
                                {
                                    // Reset the ComboBox's internal menuActive flag; the default
                                    // showPopup does this via its finished-callback, but we
                                    // bypassed it by driving the menu ourselves.
                                    if (safeThis == nullptr)
                                        return;
                                    safeThis->hidePopup();

                                    auto& processor = safeThis->processor;
                                    if (result == loadItemId)
                                    {
                                        processor.chooseModel (safeThis->getTopLevelComponent());
                                    }
                                    else if (result == clearItemId)
                                    {
                                        processor.clearModel();
                                    }
                                    else if (result == calibrateItemId)
                                    {
                                        if (auto* p = processor.getCalibrateParam())
                                        {
                                            const bool newValue = ! p->get();
                                            p->beginChangeGesture();
                                            p->setValueNotifyingHost (newValue ? 1.0f : 0.0f);
                                            p->endChangeGesture();
                                            safeThis->refreshTooltip();
                                        }
                                    }
                                });
        }

        void refreshTooltip()
        {
            if (! processor.currentModelHasCalibration())
                setTooltip ("Model not calibrated - Calibrate Input control unavailable");
            else
                setTooltip ("Model calibration: "
                            + juce::String (processor.getModelInputLevelDBu(), 1) + " dBu");
        }

        String getDisplayText() const
        {
            const auto& name = processor.getCurrentModelName();
            return name.isEmpty() ? "(no model)" : name;
        }

        void visibilityChanged() override
        {
            setName ("Model");
        }

    private:
        enum ItemIds
        {
            loadItemId = 1,
            clearItemId = 2,
            calibrateItemId = 3,
        };

        NAMProcessor& processor;
        chowdsp::ScopedCallback modelChangeCallback;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModelChoiceBox)
    };

    customComps.add (std::make_unique<ModelChoiceBox> (*this, modelChangeBroadcaster));

    juce::ignoreUnused (hcp);
    return false;
}

#endif // ! JUCE_IOS
