#pragma once

#include "../BaseProcessor.h"
#include "../utility/DCBlocker.h"

class Warp : public BaseProcessor
{
public:
    explicit Warp (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    std::atomic<float>* freqHzParam = nullptr;
    std::atomic<float>* gainDBParam = nullptr;
    std::atomic<float>* fbParam = nullptr;

    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> freqHzSmooth[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> gainDBSmooth[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> fbSmooth[2];
    chowdsp::SmoothedBufferValue<float, ValueSmoothingTypes::Linear> fbDriveSmooth;

    struct WarpFilter
    {
        struct Biquad
        {
            static constexpr int order = 2;

            inline float processSample (float x, float driveG) noexcept
            {
                float y = z[1] + x * b[0];
                float y_d = std::asinh (y * driveG) / driveG;
                z[1] = z[2] + x * b[1] - y_d * a[1];
                z[order] = x * b[order] - y_d * a[order];
                return y;
            }

            void calcCoefs (float fc, float qVal, float gain, float fs)
            {
                using namespace chowdsp::ConformalMaps;

                const auto wc = MathConstants<float>::twoPi * fc;
                const auto K = computeKValue (fc, fs);

                auto kSqTerm = 1.0f / (wc * wc);
                auto kTerm = 1.0f / (qVal * wc);

                float kNum = gain > 1.0f ? kTerm * gain : kTerm;
                float kDen = gain < 1.0f ? kTerm / gain : kTerm;

                Transform<float, 2>::bilinear (b, a, { kSqTerm, kNum, 1.0f }, { kSqTerm, kDen, 1.0f }, K);
            }

            float a[order + 1];
            float b[order + 1];
            float z[order + 1];
        } biquad {};

        void reset()
        {
            for (auto& z : biquad.z)
                z = 0.0f;

            y1 = 0.0f;
        }

        // param stuff
        float fbMult = 0.0f;
        float driveAmt = 1.0f;

        // state
        float y1 = 0.0f;
    };

    WarpFilter filter[2];

    float fs = 48000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Warp)
};
