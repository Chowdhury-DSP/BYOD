#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"
#include "../utility/DCBlocker.h"

class RONN : public BaseProcessor,
             private AudioProcessorValueTreeState::Listener
{
public:
    explicit RONN (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();
    void parameterChanged (const String& parameterID, float newValue) final;

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    // model loading utils
    SpinLock modelLoadingLock;
    void reloadModel (int randomSeed);

    // input gain
    std::atomic<float>* inGainDbParam = nullptr;
    dsp::Gain<float> inputGain;
    float makeupGain = 1.0f;

    RTNeural::ModelT<float, 1, 1, RTNeural::DenseT<float, 1, 8>, RTNeural::TanhActivationT<float, 8>, RTNeural::Conv1DT<float, 8, 4, 3, 2>, RTNeural::TanhActivationT<float, 4>, RTNeural::GRULayerT<float, 4, 8>, RTNeural::DenseT<float, 8, 1>> neuralNet[2];

    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RONN)
};
