#ifndef SUMMINGAMP_H_INCLUDED
#define SUMMINGAMP_H_INCLUDED

#include <pch.h>

namespace GainStageSpace
{
class SummingAmp : public chowdsp::IIRFilter<1>
{
public:
    SummingAmp() {}

    void prepare (float sampleRate)
    {
        chowdsp::IIRFilter<1>::reset();
        fs = (float) sampleRate;

        calcCoefs();
    }

    void calcCoefs()
    {
        constexpr float R20 = (float) 392e3;
        constexpr float C13 = (float) 820e-12;

        // analog coefficients
        float as[2], bs[2];
        bs[0] = 0.0f;
        bs[1] = R20;
        as[0] = C13 * R20;
        as[1] = 1.0f;

        const auto K = 2.0f * fs;
        chowdsp::Bilinear::BilinearTransform<float, 2>::call (b, a, bs, as, K);
    }

private:
    float fs = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SummingAmp)
};
} // namespace GainStageSpace

#endif // SUMMINGAMP_H_INCLUDED
