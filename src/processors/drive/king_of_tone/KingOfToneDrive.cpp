#include "KingOfToneDrive.h"
#include "../../ParameterHelpers.h"

namespace
{
namespace Components
{
    constexpr auto C3 = 0.01e-6f;
    constexpr auto C4 = 100e-12f;
    constexpr auto C5 = 0.01e-6f;
    constexpr auto C6 = 0.01e-6f;
    constexpr auto C7 = 0.1e-6f;
    constexpr auto R4 = 1.0e6f;
    constexpr auto R6 = 10e3f;
    constexpr auto R7 = 33e3f;
    constexpr auto R8 = 27e3f;
    constexpr auto R9 = 10e3f;
    constexpr auto R10 = 220e3f;
    constexpr auto Rp = 100e3f;
} // namespace Components

template <typename FilterType>
void calcDriveAmpCoefs (FilterType& filter, float driveParam, float fs)
{
    using namespace Components;
    const auto Rd = R6 + Rp * std::pow (driveParam, 1.5f);

    constexpr auto R1_b2 = C5 * C6 * R7 * R8;
    constexpr auto R1_b1 = C5 * R7 + C6 * R8;
    constexpr auto R1_b0 = 1.0f;
    constexpr auto R1_a2 = C5 * C6 * (R7 + R8);
    constexpr auto R1_a1 = 0.0f;
    constexpr auto R1_a0 = C5 + C6;

    const auto R2_b0 = Rd;
    const auto R2_a1 = C4 * Rd;
    constexpr auto R2_a0 = 1.0f;

    float a_s[4] {};
    a_s[0] = R1_b2 * R2_a1;
    a_s[1] = R1_b1 * R2_a1 + R1_b2 * R2_a0;
    a_s[2] = R1_b0 * R2_a1 + R1_b1 * R2_a0;
    a_s[3] = R1_b0 * R2_a0;

    float b_s[4] {};
    b_s[0] = a_s[0];
    b_s[1] = a_s[1] + R1_a2 * R2_b0;
    b_s[2] = a_s[2] + R1_a1 * R2_b0;
    b_s[3] = a_s[3] + R1_a0 * R2_b0;

    using namespace chowdsp::Bilinear;
    float b_z[4] {};
    float a_z[4] {};
    BilinearTransform<float, 4>::call (b_z, a_z, b_s, a_s, computeKValue (4000.0f, fs));
    filter.setCoefs (b_z, a_z);
}

template <typename FilterType>
void calcDriveaStageBypassedCoefs (FilterType& filter, float fs)
{
    using namespace Components;
    float b_s[2] { C7 * (R9 + R10), 1.0f };
    float a_s[2] { C7 * R9, 1.0f };

    using namespace chowdsp::Bilinear;
    float b_z[2] {};
    float a_z[2] {};
    BilinearTransform<float, 2>::call (b_z, a_z, b_s, a_s, 2.0f * fs);
    filter.setCoefs (b_z, a_z);
}
} // namespace

KingOfToneDrive::KingOfToneDrive (UndoManager* um) : BaseProcessor ("Tone King", createParameterLayout(), um)
{
    driveParam = vts.getRawParameterValue ("drive");
    modeParam = vts.getRawParameterValue ("mode");

    uiOptions.backgroundColour = Colour (0xFFAA659B);
    uiOptions.powerColour = Colour (0xFFEBD05B);
    uiOptions.info.description = "Virtual analog emulation of the drive stages from the Analogman \"King of Tone\" pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout KingOfToneDrive::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "drive", "Drive", 0.5f);
    emplace_param<AudioParameterChoice> (params, "mode", "Mode", StringArray { "Boost", "Overdrive", "Both" }, 1);

    return { params.begin(), params.end() };
}

void KingOfToneDrive::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    const auto inputFilterFreq = 1.0f / (MathConstants<float>::twoPi * Components::C3 * Components::R4);
    for (auto& filt : inputFilter)
    {
        filt.reset();
        filt.calcCoefs (inputFilterFreq, fs);
    }

    for (auto& filt : driveAmp)
    {
        filt.reset();
        calcDriveAmpCoefs (filt, *driveParam, fs);
    }

    for (auto& driveParamSm : driveParamSmooth)
    {
        driveParamSm.reset (sampleRate, 0.025);
        driveParamSm.setCurrentAndTargetValue (*driveParam);
    }

    for (auto& od : overdrive)
        od.prepare (fs);

    for (auto& filt : overdriveStageBypass)
    {
        filt.reset();
        calcDriveaStageBypassedCoefs (filt, fs);
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    prevMode = (int) *modeParam;
    prevNumChannels = 2;

    preBuffer.setSize (2, samplesPerBlock);
    doPreBuffering();
}

void KingOfToneDrive::doPreBuffering()
{
    preBuffer.setSize (prevNumChannels, preBuffer.getNumSamples(), false, false, true);
    const auto numSamples = preBuffer.getNumSamples();
    for (int i = 0; i < (int) fs; i += numSamples)
    {
        preBuffer.clear();
        processAudio (preBuffer);
    }
}

void KingOfToneDrive::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = (int) buffer.getNumSamples();
    const auto numChannels = (int) buffer.getNumChannels();

    const auto currentMode = (int) *modeParam;
    if (currentMode != prevMode || numChannels != prevNumChannels)
    {
        prevMode = currentMode;
        prevNumChannels = numChannels;
        doPreBuffering();
    }

    buffer.applyGain (0.2f); // voltage scaling

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        // process input filter
        inputFilter[ch].processBlock (x, numSamples);

        // process drive amp
        driveParamSmooth[ch].setTargetValue (*driveParam);
        if (! driveParamSmooth[ch].isSmoothing())
        {
            calcDriveAmpCoefs (driveAmp[ch], driveParamSmooth[ch].getNextValue(), fs);
            driveAmp[ch].processBlock (x, numSamples);
        }
        else
        {
            for (int n = 0; n < numSamples; ++n)
            {
                calcDriveAmpCoefs (driveAmp[ch], driveParamSmooth[ch].getNextValue(), fs);
                x[n] = driveAmp[ch].processSample (x[n]);
            }
        }

        if (currentMode == 1 || currentMode == 2) // process drive stage
            overdrive[ch].processBlock (x, numSamples);
        else // process drive stage bypassed
        {
            overdriveStageBypass[ch].processBlock (x, numSamples);
            FloatVectorOperations::multiply (x, Decibels::decibelsToGain (-30.0f), numSamples);
            FloatVectorOperations::add (x, 4.5f, numSamples);
        }

        if (currentMode == 0 || currentMode == 2) // process clipper stage
        {
            FloatVectorOperations::clip (x, x, 3.0f, 6.0f, numSamples); // clip the signal here so we don't blow out the diode models
            clipper[ch].processBlock (x, numSamples);

            const auto makeupGainDB = currentMode == 0 ? 45.0f : 27.0f;
            FloatVectorOperations::multiply (x, Decibels::decibelsToGain (makeupGainDB), numSamples);
        }
        else
        {
            FloatVectorOperations::multiply (x, Decibels::decibelsToGain (-12.0f), numSamples);
        }
    }

    dcBlocker.processAudio (buffer);
}
