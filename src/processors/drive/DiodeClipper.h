#pragma once

#include "../BaseProcessor.h"

class DiodeClipperWDF
{
public:
    DiodeClipperWDF (float sampleRate) : C1 (capVal, sampleRate) {}

    void setParameters (float cutoff)
    {
        Vs.setResistanceValue (1.0f / (MathConstants<float>::twoPi * cutoff * capVal));
    }

    void process (float* buffer, const int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
        {
            Vs.setVoltage (buffer[n]);

            dp.incident (P1.reflected());
            buffer[n] = chowdsp::WDFT::voltage<float> (C1);
            P1.incident (dp.reflected());
        }
    }

private:
    static constexpr float capVal = 47.0e-9f;
    using wdf_type = float;
    using Res = chowdsp::WDFT::ResistorT<wdf_type>;
    using Cap = chowdsp::WDFT::CapacitorT<wdf_type>;
    using ResVs = chowdsp::WDFT::ResistiveVoltageSourceT<wdf_type>;

    ResVs Vs { 4700.0f };
    Cap C1;

    chowdsp::WDFT::WDFParallelT<wdf_type, Cap, ResVs> P1 { C1, Vs };
    chowdsp::WDFT::DiodePairT<wdf_type, decltype (P1)> dp { 2.52e-9f, 0.02585f, P1 }; // GZ34 diode pair (@TODO implement other types of diodes)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiodeClipperWDF)
};

class DiodeClipper : public BaseProcessor
{
public:
    DiodeClipper (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void setGains (float driveValue);

private:
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* driveParam = nullptr;

    dsp::Gain<float> inGain, outGain;
    std::unique_ptr<DiodeClipperWDF> wdf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiodeClipper)
};
