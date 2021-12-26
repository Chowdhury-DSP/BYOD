#pragma once

#include "../BaseProcessor.h"

class Tuner : public BaseProcessor
{
public:
    explicit Tuner (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void getCustomComponents (OwnedArray<Component>& customComps) override;

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    struct TunerBackgroundTask : private Thread
    {
        TunerBackgroundTask() : Thread ("Tuner Background Task") {}

        void prepare (double sampleRate, int samplesPerBlock);

        void pushSamples (const float* samples, int numSamples);

        void run() override;

        double getCurrentFreqHz() const noexcept { return curFreqHz.load(); }

        void setShouldBeRunning (bool shouldRun);

        void computeCurrentFrequency();

    private:
        chowdsp::DoubleBuffer<float> data;
        std::atomic<int> writePosition { 0 };

        std::atomic<double> curFreqHz = { 0.0 };

        std::atomic_bool shouldBeRunning { false };
        std::atomic_bool isPrepared { false };
        double fs = 48000.0;
        int autocorrelationSize = 0;
        int waitMilliseconds = 0;
    } tunerTask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tuner)
};
