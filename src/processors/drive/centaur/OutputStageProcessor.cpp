#include "OutputStageProcessor.h"

void OutputStageProc::prepare (float sampleRate)
{
    chowdsp::IIRFilter<1>::reset();
    fs = (float) sampleRate;

    levelSmooth.setCurrentAndTargetValue (levelSmooth.getTargetValue());
    levelSmooth.reset (sampleRate, 0.05);

    calcCoefs (levelSmooth.getTargetValue());
}

void OutputStageProc::calcCoefs (float curLevel)
{
    const float R1 = 560.0f + (1.0f - curLevel) * 10000.0f;
    const float R2 = curLevel * 10000.0f + 1.0f;
    constexpr auto C1 = 4.7e-6f;

    // analog coefficients
    float as[2], bs[2];
    bs[0] = C1 * R2;
    bs[1] = 0.0f;
    as[0] = C1 * (R1 + R2);
    as[1] = 1.0f;

    // bilinear transform
    const auto K = 2.0f * fs;
    chowdsp::ConformalMaps::Transform<float, 1>::bilinear (b, a, bs, as, K);
}

void OutputStageProc::processBlock (float* block, const int numSamples) noexcept
{
    if (levelSmooth.isSmoothing())
    {
        for (int n = 0; n < numSamples; ++n)
        {
            calcCoefs (levelSmooth.getNextValue());
            block[n] = processSample (block[n]);
        }
    }
    else
    {
        calcCoefs (levelSmooth.getNextValue());
        chowdsp::IIRFilter<1>::processBlock (block, numSamples);
    }
}
