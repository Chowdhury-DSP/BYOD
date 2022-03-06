#pragma once

#include "../BaseProcessor.h"

class Tuner : public BaseProcessor
{
public:
    explicit Tuner (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void getCustomComponents (OwnedArray<Component>& customComps) override;
    void inputConnectionChanged (int portIndex, bool wasConnected) override;

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    struct TunerBackgroundTask : chowdsp::AudioUIBackgroundTask
    {
        TunerBackgroundTask() : chowdsp::AudioUIBackgroundTask ("Tuner Background Task") {}

        void prepareTask (double sampleRate, int samplesPerBlock, int& requestedBlockSize, int& waitMs) override;
        void resetTask() override;
        void runTask (const AudioBuffer<float>& data) override;

        double getCurrentFreqHz() noexcept;

    private:
        chowdsp::TunerProcessor<float> tuner;
        std::atomic<double> curFreqHz {};

        SmoothedValue<double, ValueSmoothingTypes::Multiplicative> freqValSmoother;

    } tunerTask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tuner)
};
