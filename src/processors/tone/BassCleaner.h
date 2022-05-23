#pragma once

#include "../BaseProcessor.h"

class BassCleaner : public BaseProcessor
{
public:
    explicit BassCleaner (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    inline void calcCoefs (float Rv1Val) noexcept
    {
        float b_s[] { C3 * C4 * Rv1Val, C3 + C4, 0.0f };
        float a_s[] { C3 * C4 * Rv1Val * R4, R4 * (C3 + C4) + C3 * Rv1Val, 1.0f };

        float b[3], a[3];
        chowdsp::ConformalMaps::Transform<float, 3>::bilinear (b, a, b_s, a_s, 2.0f * fs);

        b[0] *= 3200.0f;
        b[1] *= 3200.0f;
        b[2] *= 3200.0f;

        for (auto& filt : iir)
            filt.setCoefs (b, a);
    }

    static constexpr float C3 = 1.0e-6f;
    static constexpr float C4 = 47.0e-9f;
    static constexpr float R4 = 3.3e3f;
    static constexpr float Rv1Value = 50.0e3f;

    std::atomic<float>* cleanParam = nullptr;

    float fs = 48000.0f;
    SmoothedValue<float, ValueSmoothingTypes::Linear> Rv1;

    chowdsp::IIRFilter<2, float> iir[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassCleaner)
};
