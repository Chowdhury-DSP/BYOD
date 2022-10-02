#include "BlondeDrive.h"
#include "../ParameterHelpers.h"

namespace
{
const String driveTag = "drive";
const String characterTag = "char";
const String biasTag = "bias";
const String hiQTag = "high_q";

namespace Zener
{
    constexpr auto alpha = 1.0168177;
    constexpr auto log_alpha = gcem::log (alpha);
    constexpr auto beta = 9.03240196;
    constexpr auto beta_exp = beta * log_alpha;
    constexpr auto c = 0.222161;
    constexpr auto bias = 8.2;

    constexpr auto max_val = 7.5;
    constexpr auto mult = 10.0;
    constexpr auto one = 0.99;
    constexpr auto oneOverMult = one / mult;
    constexpr auto betaExpOverMult = beta_exp / mult;

    // Kind of like an approximation of std::exp(), but with some fun modifications
    // https://www.desmos.com/calculator/spplqpvc5p
    inline auto expApprox (xsimd::batch<double> x) noexcept
    {
        const auto x2 = x * x;
        const auto x3 = x2 * x;
        const auto x4 = x2 * x2;

        const auto num = x4 + 20.0 * x3 + 190.0 * x2 + 640.0 * x + 1680.0;
        const auto den = x4 - 15.0 * x3 + 180.0 * x2 - 840.0 * x + 2180.0;
        return num / den;
    }

    inline auto zener_clip_combined (xsimd::batch<double> x) noexcept
    {
        x = mult * x;

        const auto x_abs = xsimd::abs (x);
        const auto x_less_than = x_abs < max_val;

        const auto exp_out = expApprox (beta_exp * -xsimd::abs (x + c));

        const auto y = xsimd::select (x_less_than, x * oneOverMult, chowdsp::Math::sign (x) * (-exp_out + bias) * oneOverMult);
        const auto y_p = xsimd::select (x_less_than, xsimd::broadcast (one), exp_out + betaExpOverMult);

        return std::make_tuple (y, y_p);
    }
}

template <int maxIter = 8>
void processDrive (double* left, double* right, const double* A, xsimd::batch<double>& y0, const int numSamples) noexcept
{
    double stereoVec alignas (16)[2] {};
    for (int n = 0; n < numSamples; ++n)
    {
        stereoVec[0] = left[n];
        stereoVec[1] = right[n];
        auto x = xsimd::load_aligned (stereoVec);

        // Newton-Raphson loop
        for (int i = 0; i < maxIter; ++i)
        {
            const auto [zener_f, zener_fp] = Zener::zener_clip_combined (x + A[n] * y0);
            const auto F = zener_f + y0;
            const auto F_p = zener_fp * A[n] + 1.0;
            y0 -= F / F_p;
        }

        xsimd::store_aligned (stereoVec, -y0);
        left[n] = stereoVec[0];
        right[n] = stereoVec[1];
    }
}
} // namespace

BlondeDrive::BlondeDrive (UndoManager* um) : BaseProcessor ("Blonde Drive", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (characterParam, vts, characterTag);
    loadParameterPointer (hiQParam, vts, hiQTag);

    addPopupMenuParameter (hiQTag);

    driveParamSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, driveTag));
    driveParamSmooth.mappingFunction = [] (float x)
    { return 1.0f - x; };

    biasParamSmooth.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, biasTag));
    biasParamSmooth.mappingFunction = [] (float x)
    { return x * 0.5f; };

    uiOptions.backgroundColour = Colour { 0xfffcd2a4 };
    uiOptions.powerColour = Colour { 0xfff4702e }.darker (0.1f);
    uiOptions.info.description = "Drive stage based on the drive circuit from the Joyo American Sound.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout BlondeDrive::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, driveTag, "Drive", 0.5f);
    createBipolarPercentParameter (params, characterTag, "Character", 0.0f);
    createPercentParameter (params, biasTag, "Bias", 0.5f);
    emplace_param<chowdsp::BoolParameter> (params, hiQTag, "High Quality", false);

    return { params.begin(), params.end() };
}

void BlondeDrive::prepare (double sampleRate, int samplesPerBlock)
{
    driveParamSmooth.prepare (sampleRate, samplesPerBlock);
    biasParamSmooth.prepare (sampleRate, samplesPerBlock);

    doubleBuffer.setSize (2, samplesPerBlock);

    state = 0.0;

    characterFilter.setQValue (0.45f);
    characterFilter.setCutoffFrequency (685.0f);
    characterFilter.setGainDecibels (0.0f);
    characterFilter.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 });

    dcBlocker.prepare (2);
    dcBlocker.calcCoefs (30.0f, (float) sampleRate);
}

void BlondeDrive::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    driveParamSmooth.process (numSamples);
    biasParamSmooth.process (numSamples);

    characterFilter.setGainDecibels (*characterParam * 12.0f);
    characterFilter.processBlock (buffer);

    buffer.applyGain (4.0f);

    // apply bias
    for (int ch = 0; ch < numChannels; ++ch)
        FloatVectorOperations::add (buffer.getWritePointer (ch), biasParamSmooth.getSmoothedBuffer(), numSamples);

    doubleBuffer.makeCopyOf (buffer, true);

    auto* driveData = driveParamSmooth.getSmoothedBuffer();
    auto* leftData = doubleBuffer.getWritePointer (0);
    auto* rightData = doubleBuffer.getWritePointer (1 % numChannels);

    if (hiQParam->get())
        processDrive<12> (leftData, rightData, driveData, state, numSamples);
    else
        processDrive (leftData, rightData, driveData, state, numSamples);

    buffer.makeCopyOf (doubleBuffer, true);

    // undo bias
    for (int ch = 0; ch < numChannels; ++ch)
        FloatVectorOperations::subtract (buffer.getWritePointer (ch), biasParamSmooth.getSmoothedBuffer(), numSamples);

    buffer.applyGain (0.7071f);

    dcBlocker.processBlock (buffer);
}
