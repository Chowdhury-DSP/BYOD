#include "InputBufferProcessor.h"

void InputBufferProcessor::prepare (float sampleRate)
{
    chowdsp::IIRFilter<1>::reset();
    fs = sampleRate;

    calcCoefs();
}

void InputBufferProcessor::calcCoefs()
{
    constexpr float R1 = 10000.0f;
    constexpr float R2 = 1000000.0f;
    constexpr float C1 = (float) 0.1e-6;

    // analog coefficients
    float as[2], bs[2];
    bs[0] = C1 * R2;
    bs[1] = 0.0f;
    as[0] = C1 * (R1 + R2);
    as[1] = 1.0f;

    // bilinear transform
    const auto K = 2.0f * fs;
    chowdsp::Bilinear::BilinearTransform<float, 2>::call (b, a, bs, as, K);
}

void InputBufferProcessor::processBlock (float* buffer, const int numSamples) noexcept
{
    chowdsp::IIRFilter<1>::processBlock (buffer, numSamples);
    // FloatVectorOperations::add (buffer, 4.5, numSamples); // bias
}
