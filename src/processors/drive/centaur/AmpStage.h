#ifndef AMPSTAGE_H_INCLUDED
#define AMPSTAGE_H_INCLUDED

#include <pch.h>

namespace GainStageSpace
{
class AmpStage : public chowdsp::IIRFilter<2>
{
public:
    AmpStage()
    {
        r10bSmooth = 2000.0f;
    }

    void setGain (float gain)
    {
        float newR10b = (1.0f - gain) * 100000.0f + 2000.0f;
        r10bSmooth.setTargetValue (jlimit (2000.0f, 102000.0f, newR10b));
    }

    void prepare (float sampleRate)
    {
        chowdsp::IIRFilter<2>::reset();
        fs = (float) sampleRate;

        r10bSmooth.setCurrentAndTargetValue (r10bSmooth.getTargetValue());
        r10bSmooth.reset (sampleRate, 0.05);

        calcCoefs (r10bSmooth.getTargetValue());
    }

    void calcCoefs (float curR10b)
    {
        // component values
        constexpr auto C7 = 82e-9f;
        constexpr auto C8 = 390e-12f;

        // analog coeffs
        float as[3], bs[3];
        as[0] = C7 * C8 * curR10b * R11 * R12;
        as[1] = C7 * curR10b * R11 + C8 * R12 * (curR10b + R11);
        as[2] = curR10b + R11;
        bs[0] = as[0];
        bs[1] = C7 * R11 * R12 + as[1];
        bs[2] = R12 + as[2];

        // frequency warping
        const float wc = chowdsp::Bilinear::calcPoleFreq (as[0], as[1], as[2]);
        const auto K = wc == 0.0f ? 2.0f * fs : wc / std::tan (wc / (2.0f * fs));

        chowdsp::Bilinear::BilinearTransform<float, 3>::call (b, a, bs, as, K);
    }

    void processBlock (float* block, const int numSamples) noexcept override
    {
        if (r10bSmooth.isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                calcCoefs (r10bSmooth.getNextValue());
                block[n] = processSample (block[n]);

                // block[n] += 4.5f * R12 / (R12 + R11 + curR10b); // bias
            }
        }
        else
        {
            calcCoefs (r10bSmooth.getNextValue());
            chowdsp::IIRFilter<2>::processBlock (block, numSamples);
            // FloatVectorOperations::add (block, 4.5f * R12 / (R12 + R11 + r10bSmooth.getCurrentValue()), numSamples); // bias
        }
    }

private:
    const float R11 = (float) 15e3;
    const float R12 = (float) 422e3;

    float fs = 44100.0f;
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> r10bSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpStage)
};
} // namespace GainStageSpace

#endif // AMPSTAGE_H_INCLUDED
