#pragma once

#if ! JUCE_IOS

#include "../../BaseProcessor.h"

namespace NeuralAudio
{
class NeuralModel;
}

class NAMProcessor : public BaseProcessor
{
public:
    explicit NAMProcessor (UndoManager* um = nullptr);
    ~NAMProcessor() override;

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    std::unique_ptr<XmlElement> toXML() override;
    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp) override;

    /**
     * Launch the file chooser to pick a model. If @p parent is non-null, the
     * chooser will be modal to that component.
     */
    void chooseModel (Component* parent);

    /**
     * Load a model from an absolute file path. Runs synchronously.
     * On failure, the processor is left in a "no model" pass-through state.
     */
    void loadModelFromFile (const File& file, Component* parent = nullptr);

    /** Unloads the current model. Safe to call from the message thread. */
    void clearModel();

    const String& getCurrentModelName() const { return currentModelName; }

    /** True if the currently loaded model exposes input_level_dbu metadata. */
    bool currentModelHasCalibration() const { return modelHasCalibration; }

    /** The dBu level the model was captured at (only valid if calibrated). */
    float getModelInputLevelDBu() const { return modelInputLevelDBu; }

    /** Access to the Calibrate toggle parameter (for the UI popup). */
    chowdsp::BoolParameter* getCalibrateParam() const { return calibrateParam; }

private:
    struct ModelLoadResult
    {
        std::array<std::unique_ptr<NeuralAudio::NeuralModel>, 2> models {};
        bool hasCalibration = false;
        float modelInputLevelDBu = 12.0f;
        float calOutputAdjustDB = 0.0f;
    };

    /** Builds both channel models for @p path at the current sample rate /
     *  block size and probes calibration metadata. Throws std::runtime_error
     *  on any failure; on success the returned result is fully populated.
     */
    ModelLoadResult buildModelsForPath (const String& path);

    // Broadcaster used to notify the UI when the loaded model changes.
    using ModelChangeBroadcaster = chowdsp::Broadcaster<void()>;
    ModelChangeBroadcaster modelChangeBroadcaster;

    chowdsp::FloatParameter* inputGainParam = nullptr;
    chowdsp::FloatParameter* outputGainParam = nullptr;
    chowdsp::FloatParameter* qualityParam = nullptr;
    chowdsp::FloatParameter* inputCalDBuParam = nullptr;
    chowdsp::BoolParameter* calibrateParam = nullptr;

    std::array<std::unique_ptr<NeuralAudio::NeuralModel>, 2> models {};
    SpinLock modelChangingMutex;

    // Max block size the *live* models were built for (0 if no models).
    // NeuralAudio models must never be given more samples per Process() call
    // than the max buffer size they were created with: the library's only
    // guard in release builds is an assert, and SetMaxAudioBufferSize() is
    // not realtime-safe. This can diverge from processMaxBlockSize when a
    // prepare()-time reload fails and the stale models are kept running.
    // Guarded by modelChangingMutex, alongside the models themselves.
    int modelsMaxBlockSize = 0;

    String cachedModelPath; // absolute path, empty if no model loaded
    String currentModelName;

    // Calibration info cached at model load time. modelHasCalibration gates
    // whether the values are applied in processAudio; the numeric fields are
    // always populated (defaults are used when the model lacks metadata).
    bool modelHasCalibration = false;
    float modelInputLevelDBu = 12.0f;   // dBu the amp was captured at
    float calOutputAdjustDB = 0.0f;     // -18 - modelLoudnessDB

    std::shared_ptr<FileChooser> modelChooser;

    chowdsp::Gain<float> inGain, outGain;
    chowdsp::FirstOrderHPF<float> dcBlocker;

    double processSampleRate = 48000.0;
    int processMaxBlockSize = 512;
    float lastQualityApplied = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NAMProcessor)
};

#endif // ! JUCE_IOS
