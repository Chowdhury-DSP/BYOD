#pragma once

#include "../BaseProcessor.h"

class HighCut : public BaseProcessor
{
public:
    explicit HighCut (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

private:
    inline void calcCoefs (float Rv2Val) noexcept
    {
        float b_s[] { 0.0, 1.0f };
        float a_s[] { (R3 + Rv2Val) * C8, 1.0f };

        float fc = 1.0f / (MathConstants<float>::twoPi * ((R3 + Rv2Val) * C8));
        float K = fc / std::tanh (fc / (2.0f * fs));

        float b[2];
        float a[2];
        chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b, a, b_s, a_s, K);

        for (auto& filt : iir)
            filt.setCoefs (b, a);
    }

    static constexpr float C8 = 10.0e-9f;
    static constexpr float R3 = 1.5e3f;

    chowdsp::FloatParameter* cutoffParam = nullptr;

    float fs = 48000.0f;
    SmoothedValue<float, ValueSmoothingTypes::Linear> Rv2;

    chowdsp::IIRFilter<1, float> iir[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HighCut)
};
