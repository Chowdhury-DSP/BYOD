#include "MXRDistortion.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace MXRDistortionParams
{
static float paramSkew (float paramVal)
{
    using namespace ParameterHelpers;
    return 1.0f - iLogPot (iLogPot (0.5f * paramVal + 0.5f));
}

const auto levelSkew = ParameterHelpers::createNormalisableRange (-60.0f, 0.0f, -12.0f);
} // namespace

MXRDistortion::MXRDistortion (UndoManager* um) : BaseProcessor ("Distortion Plus", createParameterLayout(), um)
{
    loadParameterPointer (distParam, vts, "dist");
    loadParameterPointer (levelParam, vts, "level");

    uiOptions.backgroundColour = Colours::teal;
    uiOptions.info.description = "Virtual analog emulation of the MXR Distortion+ overdrive pedal.";
    uiOptions.info.authors = StringArray { "Sam Schachter", "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::mxr_distortion_schematic_svg,
                                               .size = BinaryData::mxr_distortion_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R1",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R1_C2.setResistanceValue (self.value.load());
        },
        100.0f,
        500.0e3f);
    netlistCircuitQuantities->addResistor (
        1.0e6f,
        "R2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.Vb.setResistanceValue (self.value.load());
        },
        10.0e3f,
        10.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e6f,
        "R4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R4.setResistanceValue (self.value.load());
        },
        10.0e3f,
        10.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R5_C4.setResistanceValue (self.value.load());
        },
        100.0f,
        500.0e3f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-9f,
        "C1",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C1.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        500.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        10.0e-9f,
        "C2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R1_C2.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        500.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        47.0e-9f,
        "C3",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.ResDist_R3_C3.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        500.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-6f,
        "C4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R5_C4.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        500.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-9f,
        "C5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C5.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        500.0e-3f);
}

ParamLayout MXRDistortion::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createPercentParameter (params, "dist", "Distortion", 0.5f);
    createPercentParameter (params, "level", "Level", 0.5f);

    return { params.begin(), params.end() };
}

void MXRDistortion::prepare (double sampleRate, int samplesPerBlock)
{
    for (auto& wdfProc : wdf)
    {
        wdfProc.prepare (sampleRate);
        wdfProc.setParams (MXRDistortionParams::paramSkew (*distParam));
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    gain.prepare (spec);
    gain.setRampDurationSeconds (0.01);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 15000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void MXRDistortion::processAudio (AudioBuffer<float>& buffer)
{
    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParams (MXRDistortionParams::paramSkew (*distParam));

        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
            x[n] = wdf[ch].processSample (x[n]);
    }

    dcBlocker.processAudio (buffer);

    gain.setGainLinear (Decibels::decibelsToGain (MXRDistortionParams::levelSkew.convertFrom0to1 (*levelParam), MXRDistortionParams::levelSkew.start));
    gain.process (context);
}
