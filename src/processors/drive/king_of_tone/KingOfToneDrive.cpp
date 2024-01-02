#include "KingOfToneDrive.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace ToneKingCoeffs
{
template <typename FilterType>
void calcDriveAmpCoefs (FilterType& filter, float driveParam, float fs, const KingOfToneDrive::Components& components)
{
    const auto Rd = components.R6 + components.Rp * std::pow (driveParam, 1.5f);

    const auto R1_b2 = components.C5 * components.C6 * components.R7 * components.R8;
    const auto R1_b1 = components.C5 * components.R7 + components.C6 * components.R8;
    const auto R1_b0 = 1.0f;
    const auto R1_a2 = components.C5 * components.C6 * (components.R7 + components.R8);
    const auto R1_a1 = 0.0f;
    const auto R1_a0 = components.C5 + components.C6;

    const auto R2_b0 = Rd;
    const auto R2_a1 = components.C4 * Rd;
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

    using namespace chowdsp::ConformalMaps;
    float b_z[4] {};
    float a_z[4] {};
    Transform<float, 3>::bilinear (b_z, a_z, b_s, a_s, computeKValue (4000.0f, fs));
    filter.setCoefs (b_z, a_z);
}

template <typename FilterType>
void calcDriveStageBypassedCoefs (FilterType& filter, float fs, const KingOfToneDrive::Components& components)
{
    float b_s[2] { components.C7 * (components.R9 + components.R10), 1.0f };
    float a_s[2] { components.C7 * components.R9, 1.0f };

    using namespace chowdsp::ConformalMaps;
    float b_z[2] {};
    float a_z[2] {};
    Transform<float, 1>::bilinear (b_z, a_z, b_s, a_s, 2.0f * fs);
    filter.setCoefs (b_z, a_z);
}
} // namespace ToneKingCoeffs

KingOfToneDrive::KingOfToneDrive (UndoManager* um) : BaseProcessor ("Tone King", createParameterLayout(), um)
{
    chowdsp::ParamUtils::loadParameterPointer (driveParam, vts, "drive");
    modeParam = vts.getRawParameterValue ("mode");

    uiOptions.backgroundColour = Colour (0xFFAA659B);
    uiOptions.powerColour = Colour (0xFFEBD05B);
    uiOptions.info.description = "Virtual analog emulation of the drive stages from the Analogman \"King of Tone\" pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::king_of_tone_schematic_svg,
                                               .size = BinaryData::king_of_tone_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        1.0e6f,
        "R4",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.R4 = self.value.load();
            const auto inputFilterFreq = 1.0f / (MathConstants<float>::twoPi * components.C3 * components.R4);
            for (auto& filt : inputFilter)
            {
                filt.calcCoefs (inputFilterFreq, fs);
            }
        },
        1.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R6",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.R6 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        33.0e3f,
        "R7",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.R7 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        27.0e3f,
        "R8",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.R8 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R9",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.R9 = self.value.load();
            for (auto& filt : overdriveStageBypass)
                ToneKingCoeffs::calcDriveStageBypassedCoefs (filt, fs, components);
            for (auto& wdf : overdrive)
                wdf.R9_C7_Vin.setResistanceValue (components.R9);
        },
        100.0f,
        25.0e3f);
    netlistCircuitQuantities->addResistor (
        220.0e3f,
        "R10",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.R10 = self.value.load();
            for (auto& filt : overdriveStageBypass)
                ToneKingCoeffs::calcDriveStageBypassedCoefs (filt, fs, components);
            for (auto& wdf : overdrive)
                wdf.R10.setResistanceValue (components.R10);
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        6.8e3f,
        "R11",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : overdrive)
                wdf.R11.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R12",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : clipper)
                wdf.R12_Vs.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addCapacitor (
        0.01e-6f,
        "C3",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.R4 = self.value.load();
            const auto inputFilterFreq = 1.0f / (MathConstants<float>::twoPi * components.C3 * components.R4);
            for (auto& filt : inputFilter)
                filt.calcCoefs (inputFilterFreq, fs);
        },
        10.0e-9f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        100.0e-12f,
        "C4",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.C4 = self.value.load();
        },
        1.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        0.01e-6f,
        "C5",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.C5 = self.value.load();
        },
        1.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        0.01e-6f,
        "C6",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.C6 = self.value.load();
        },
        1.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        0.1e-6f,
        "C7",
        [this] (const netlist::CircuitQuantity& self)
        {
            components.C7 = self.value.load();
            for (auto& filt : overdriveStageBypass)
                ToneKingCoeffs::calcDriveStageBypassedCoefs (filt, fs, components);
            for (auto& wdf : overdrive)
                wdf.R9_C7_Vin.setCapacitanceValue (components.C7);
        },
        1.0e-9f,
        1.0e-3f);
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

    const auto inputFilterFreq = 1.0f / (MathConstants<float>::twoPi * components.C3 * components.R4);
    for (auto& filt : inputFilter)
    {
        filt.reset();
        filt.calcCoefs (inputFilterFreq, fs);
    }

    for (auto& filt : driveAmp)
    {
        filt.reset();
        ToneKingCoeffs::calcDriveAmpCoefs (filt, *driveParam, fs, components);
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
        ToneKingCoeffs::calcDriveStageBypassedCoefs (filt, fs, components);
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
            ToneKingCoeffs::calcDriveAmpCoefs (driveAmp[ch], driveParamSmooth[ch].getNextValue(), fs, components);
            driveAmp[ch].processBlock (x, numSamples);
        }
        else
        {
            for (int n = 0; n < numSamples; ++n)
            {
                ToneKingCoeffs::calcDriveAmpCoefs (driveAmp[ch], driveParamSmooth[ch].getNextValue(), fs, components);
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
