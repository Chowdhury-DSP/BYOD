#pragma once

#include "../BaseProcessor.h"

class BigMuffTone : public BaseProcessor
{
public:
    explicit BigMuffTone (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    struct Components
    {
        const std::string_view name;

        // default values: Ram's Head '75
        const float R8 = 39.0e3f;
        const float C8 = 10.0e-9f;
        const float C9 = 4.0e-9f;

        const float R5 = 22.0e3f;
        const float R5_1 = 0.35f * R5;
        const float R5_2 = 2.0f * (R5 - R5_1); // so that 50% = R5

        const float RT = 100.0e3f;
    };

private:
    inline void calcCoefs (float tone, float mids, const Components& c)
    {
        const auto R5 = c.R5_1 + mids * c.R5_2;
        const auto Za = tone * c.RT;
        const auto Zb = (1 - tone) * c.RT;

        const auto phi = c.C8 * c.C9 * R5 * c.R8;
        const auto b2 = phi * Za;
        const auto b1 = c.C9 * R5 * (c.R8 + c.RT);
        const auto b0 = R5 + Zb;
        const auto a2 = phi * (Za + Zb);
        const auto a1 = c.C8 * c.R8 * (R5 + c.RT) + b1;
        const auto a0 = R5 + c.R8 + c.RT;

        using namespace chowdsp::ConformalMaps;
        float fLP = 1.0f / (MathConstants<float>::twoPi * c.R8 * c.C8);
        float fHP = 1.0f / (MathConstants<float>::twoPi * R5 * c.C9);
        float fc = std::sqrt (fLP * fHP);
        float K = computeKValue (fc, fs);

        float b[3];
        float a[3];
        Transform<float, 2>::bilinear (b, a, { b2, b1, b0 }, { a2, a1, a0 }, K);

        for (auto& filt : iir)
            filt.setCoefs (b, a);
    }

    chowdsp::FloatParameter* toneParam = nullptr;
    chowdsp::FloatParameter* midsParam = nullptr;
    std::atomic<float>* typeParam = nullptr;

    float fs = 48000.0f;
    chowdsp::IIRFilter<2, float> iir[2];

    const Components* comps;
    SmoothedValue<float, ValueSmoothingTypes::Linear> toneSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> midsSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigMuffTone)
};
