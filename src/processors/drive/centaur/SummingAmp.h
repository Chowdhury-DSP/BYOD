#ifndef SUMMINGAMP_H_INCLUDED
#define SUMMINGAMP_H_INCLUDED

#include <pch.h>

namespace GainStageSpace
{
class SummingAmp : public chowdsp::IIRFilter<1>
{
public:
    SummingAmp() = default;

    void prepare (float sampleRate)
    {
        chowdsp::IIRFilter<1>::reset();
        fs = (float) sampleRate;

        calcCoefs();
    }

    void calcCoefs()
    {
        constexpr auto R20 = 392e3f;
        constexpr auto C13 = 820e-12f;

        // analog coefficients
        float as[2], bs[2];
        bs[0] = 0.0f;
        bs[1] = R20;
        as[0] = C13 * R20;
        as[1] = 1.0f;

        const auto K = 2.0f * fs;
        chowdsp::ConformalMaps::Transform<float, 2>::bilinear (b, a, bs, as, K);
    }

private:
    float fs = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SummingAmp)
};
} // namespace GainStageSpace

#endif // SUMMINGAMP_H_INCLUDED
