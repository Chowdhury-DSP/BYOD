#pragma once

#include "../../BaseProcessor.h"
#include "../../utility/DCBlocker.h"
#include "KingOfToneClipper.h"
#include "KingOfToneOverdrive.h"

class KingOfToneDrive : public BaseProcessor
{
public:
    explicit KingOfToneDrive (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    struct Components
    {
        float C3 = 0.01e-6f;
        float C4 = 100e-12f;
        float C5 = 0.01e-6f;
        float C6 = 0.01e-6f;
        float C7 = 0.1e-6f;
        float R4 = 1.0e6f;
        float R6 = 10e3f;
        float R7 = 33e3f;
        float R8 = 27e3f;
        float R9 = 10e3f;
        float R10 = 220e3f;
        static constexpr auto Rp = 100e3f;
    } components;

private:
    void doPreBuffering();

    chowdsp::FloatParameter* driveParam = nullptr;
    std::atomic<float>* modeParam = nullptr;

    SmoothedValue<float> driveParamSmooth[2];
    int prevMode = 0;
    int prevNumChannels = 0;

    float fs = 48000.0f;
    chowdsp::FirstOrderHPF<float> inputFilter[2];
    chowdsp::IIRFilter<3, float> driveAmp[2];
    chowdsp::IIRFilter<1, float> overdriveStageBypass[2];
    KingOfToneOverdrive overdrive[2];
    KingOfToneClipper clipper[2];

    AudioBuffer<float> preBuffer;
    DCBlocker dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KingOfToneDrive)
};
