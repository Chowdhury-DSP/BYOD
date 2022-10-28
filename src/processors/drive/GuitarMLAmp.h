#pragma once

#include "../BaseProcessor.h"
#include "../utility/DCBlocker.h"
#include "neural_utils/ResampledRNN.h"

class GuitarMLAmp : public BaseProcessor,
                    private AudioProcessorValueTreeState::Listener
{
public:
    explicit GuitarMLAmp (UndoManager* um = nullptr);
    ~GuitarMLAmp() override;

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    std::unique_ptr<XmlElement> toXML() override;
    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

private:
    void parameterChanged (const String& paramID, float newValue) override;
    void loadModel (int modelIndex);
    void loadModelFromJson (const chowdsp::json& modelJson);

    chowdsp::FloatParameter* gainParam = nullptr;
    chowdsp::Gain<float> inGain;

    std::atomic<float>* modelParam = nullptr;
    SpinLock modelChangingMutex;
    double processSampleRate = 96000.0;
    bool loadCachedCustomModel = false;
    std::shared_ptr<FileChooser> customModelChooser;

    template <int numIns, int hiddenSize>
    using GuitarML_LSTM = RTNeural::ModelT<float, numIns, 1, RTNeural::LSTMLayerT<float, numIns, hiddenSize, RTNeural::SampleRateCorrectionMode::LinInterp>, RTNeural::DenseT<float, hiddenSize, 1>>;

    using LSTM40Cond = GuitarML_LSTM<2, 40>;
    using LSTM40NoCond = GuitarML_LSTM<1, 40>;

    std::array<LSTM40Cond, 2> lstm40CondModels;
    std::array<LSTM40NoCond, 2> lstm40NoCondModels;

    struct ModelConfig
    {
        enum ModelType
        {
            LSTM40Cond,
            LSTM40NoCond,
        };

        ModelType modelType = LSTM40NoCond;
        double trainingSampleRate = 44100.0;
    } modelConfig {};

    chowdsp::json cachedCustomModel {};

    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarMLAmp)
};
