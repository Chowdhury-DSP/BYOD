#pragma once

#include "neural_utils/ResampledRNNAccelerated.h"

#include "../BaseProcessor.h"
#include "../utility/DCBlocker.h"

class GuitarMLAmp : public BaseProcessor
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

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp) override;
    void addToPopupMenu (PopupMenu& menu) override;

    void loadModel (int modelIndex, Component* parentComponent = nullptr);
    String getCurrentModelName() const;

private:
    void loadModelFromJson (const chowdsp::json& modelJson, const String& newModelName = {});
    using ModelChangeBroadcaster = chowdsp::Broadcaster<void()>;
    ModelChangeBroadcaster modelChangeBroadcaster;

    chowdsp::FloatParameter* gainParam = nullptr;
    chowdsp::SmoothedBufferValue<float> conditionParam;
    chowdsp::BoolParameter* sampleRateCorrectionFilterParam = nullptr;
    chowdsp::Gain<float> inGain;

    SpinLock modelChangingMutex;
    double processSampleRate = 96000.0;
    std::shared_ptr<FileChooser> customModelChooser;

    template <int numIns, int hiddenSize>
    using GuitarML_LSTM = EA::Variant<rnn_sse_arm::RNNAccelerated<numIns, hiddenSize, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>
#if JUCE_INTEL
                                      ,
                                      rnn_avx::RNNAccelerated<numIns, hiddenSize, RecurrentLayerType::LSTMLayer, (int) RTNeural::SampleRateCorrectionMode::LinInterp>
#endif
                                      >;

    using LSTM40Cond = GuitarML_LSTM<2, 40>;
    using LSTM40NoCond = GuitarML_LSTM<1, 40>;

    std::array<LSTM40Cond, 2> lstm40CondModels;
    std::array<LSTM40NoCond, 2> lstm40NoCondModels;
    chowdsp::HighShelfFilter<float> sampleRateCorrectionFilter;

    enum class ModelArch
    {
        LSTM40Cond,
        LSTM40NoCond,
    };

    ModelArch modelArch = ModelArch::LSTM40NoCond;

    chowdsp::json cachedModel {};

    DCBlocker dcBlocker;

    float normalizationGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarMLAmp)
};
