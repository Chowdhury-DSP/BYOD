#include "Warp.h"
#include "../ParameterHelpers.h"

namespace
{
inline float f_NL (float x, float fbDrive) noexcept
{
    return std::tanh (x) / fbDrive;
}

inline float f_NL_prime (float x, float fbDrive) noexcept
{
    const auto coshX = std::cosh (x);
    return 1.0f / (coshX * coshX * fbDrive);
}

inline float func (float x, float y, float yDrive, float Hn, float h0, float G, float fbDrive) noexcept
{
    return y - h0 * (x + G * f_NL (yDrive, fbDrive)) - Hn;
}

inline float func_deriv (float yDrive, float h0, float G, float fbDrive) noexcept
{
    return 1.0f - h0 * G * f_NL_prime (yDrive, fbDrive);
}

template <int MAX_ITER>
inline std::pair<float, float> newton_raphson (float x, float y1, float b, float z, float fbAmount, float fbDrive)
{
    for (size_t k = 0; k < MAX_ITER; ++k)
    {
        const auto yDrive = y1 * fbDrive;
        const auto delta = -1.0f * func (x, y1, yDrive, z, b, fbAmount, fbDrive) / func_deriv (yDrive, b, fbAmount, fbDrive);

        y1 += delta;
    }

    float y0 = x + fbAmount * f_NL (y1 * fbDrive, fbDrive);
    return std::make_pair (y0, y1);
}

/**
 * Q to gain relationship for adaptive Q.
 * The polynomial here is derived in sim/GraphicEQ/adaptive_q.py
 */
constexpr float calcQ (float gainDB)
{
    constexpr float adaptiveQCoeffs[] = { -7.75358366e-09f, 5.21182270e-23f, 2.70080663e-06f, -3.04753193e-20f, -3.29851878e-04f, 1.89860352e-18f, 2.59076683e-02f, -4.77485061e-17f, 3.78416236e-01f };
    return chowdsp::Polynomials::estrin<8> (adaptiveQCoeffs, gainDB);
}
} // namespace

Warp::Warp (UndoManager* um) : BaseProcessor ("Warp", createParameterLayout(), um)
{
    freqHzParam = vts.getRawParameterValue ("freq");
    gainDBParam = vts.getRawParameterValue ("gain");
    fbParam = vts.getRawParameterValue ("fb");
    fbDriveSmooth.setParameterHandle (vts.getRawParameterValue ("fb_drive"));

    uiOptions.backgroundColour = Colour (0xffa713e2);
    uiOptions.powerColour = Colours::cyan;
    uiOptions.info.description = "Drive effect based on nonlinear feedback filters.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Warp::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createFreqParameter (params, "freq", "Freq", 100.0f, 1000.0f, 250.0f, 250.0f);
    createGainDBParameter (params, "gain", "Gain", 0.0f, 12.0f, 6.0f);
    createPercentParameter (params, "fb", "Feedback", 0.5f);
    emplace_param<VTSParam> (params, "fb_drive", "FB Drive", String(), createNormalisableRange (1.0f, 10.0f, 5.0f), 5.0f, &floatValToString, &stringToFloatVal);

    return { params.begin(), params.end() };
}

void Warp::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    for (auto& f : filter)
        f.reset();

    for (int ch = 0; ch < 2; ++ch)
    {
        freqHzSmooth[ch].reset (sampleRate, 0.05);
        gainDBSmooth[ch].reset (sampleRate, 0.05);
        fbSmooth[ch].reset (sampleRate, 0.05);
    }

    fbDriveSmooth.prepare (sampleRate, samplesPerBlock);
    fbDriveSmooth.setRampLength (0.05);
}

void Warp::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    auto cookParams = [&] (int ch, auto& filt)
    {
        const auto freqHz = freqHzSmooth[ch].getNextValue();
        const auto gainDB = gainDBSmooth[ch].getNextValue();
        filt.biquad.calcCoefs (freqHz, calcQ (gainDB), Decibels::decibelsToGain (gainDB), fs);
        filt.fbMult = fbSmooth[ch].getNextValue();
        filt.driveAmt = 0.1f * (freqHz / fs) * (96000.0f / 500.0f);
    };

    auto processSample = [] (float x, float fbDrive, auto& f)
    {
        auto [y0, y1] = newton_raphson<4> (x, f.y1, f.biquad.b[0], f.biquad.z[1], f.fbMult, fbDrive);

        f.biquad.processSample (y0, f.driveAmt);
        f.y1 = y1;

        return std::tanh (y1 * 0.5f);
    };

    fbDriveSmooth.process (numSamples);
    const auto* fbDriveData = fbDriveSmooth.getSmoothedBuffer();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        freqHzSmooth[ch].setTargetValue (*freqHzParam);
        gainDBSmooth[ch].setTargetValue (*gainDBParam);
        fbSmooth[ch].setTargetValue (*fbParam * -0.95f);

        const auto isSmoothing = freqHzSmooth[ch].isSmoothing() || gainDBSmooth[ch].isSmoothing() || fbSmooth[ch].isSmoothing();
        auto* x = buffer.getWritePointer (ch);
        FloatVectorOperations::multiply (x, 5.0f, numSamples);

        auto& filt = filter[ch];
        if (! isSmoothing)
        {
            cookParams (ch, filt);
            for (int n = 0; n < numSamples; ++n)
                x[n] = processSample (x[n], fbDriveData[n], filt);
        }
        else
        {
            for (int n = 0; n < numSamples; ++n)
            {
                cookParams (ch, filt);
                x[n] = processSample (x[n], fbDriveData[n], filt);
            }
        }
    }
}
