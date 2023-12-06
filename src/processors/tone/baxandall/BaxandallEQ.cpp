#include "BaxandallEQ.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace BaxandallParams
{
float skewParam (float val)
{
    val = std::pow (val, 3.333f);
    return jlimit (0.01f, 0.99f, 1.0f - val);
}
} // namespace

BaxandallEQ::BaxandallEQ (UndoManager* um) : BaseProcessor ("Baxandall EQ", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (bassParam, vts, "bass");
    loadParameterPointer (trebleParam, vts, "treble");

    uiOptions.backgroundColour = Colours::seagreen;
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "An EQ filter based on Baxandall EQ circuit.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::baxandall_eq_schematic_svg,
                                               .size = BinaryData::baxandall_eq_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "Ra",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Resa.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "Rb",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Resb.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "Rc",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Resc.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "Rd",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Resd = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "Re",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Rese = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e6f,
        "RL",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Rl.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-6f,
        "Ca",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Ca.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        22.0e-9f,
        "Cb",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Pb_plus_Cb.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        220.0e-9f,
        "Cc",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Pb_minus_Cc.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        6.4e-9f,
        "Cd",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Pt_plus_Resd_Cd.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        64.0e-9f,
        "Ce",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdfCircuit)
                wdfModel.Pt_minus_Rese_Ce.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
}

ParamLayout BaxandallEQ::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "bass", "Bass", 0.5f);
    createPercentParameter (params, "treble", "Treble", 0.5f);

    return { params.begin(), params.end() };
}

void BaxandallEQ::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        wdfCircuit[ch].prepare (sampleRate);

        bassSmooth[ch].reset (sampleRate, 0.05);
        bassSmooth[ch].setCurrentAndTargetValue (BaxandallParams::skewParam (*bassParam));

        trebleSmooth[ch].reset (sampleRate, 0.05);
        trebleSmooth[ch].setCurrentAndTargetValue (BaxandallParams::skewParam (*trebleParam));
    }
}

void BaxandallEQ::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        bassSmooth[ch].setTargetValue (BaxandallParams::skewParam (*bassParam));
        trebleSmooth[ch].setTargetValue (BaxandallParams::skewParam (*trebleParam));

        auto* x = buffer.getWritePointer (ch);
        if (bassSmooth[ch].isSmoothing() || trebleSmooth[ch].isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                wdfCircuit[ch].setParams (bassSmooth[ch].getNextValue(), trebleSmooth[ch].getNextValue());
                x[n] = wdfCircuit[ch].processSample (x[n]);
            }
        }
        else
        {
            wdfCircuit[ch].setParams (bassSmooth[ch].getNextValue(), trebleSmooth[ch].getNextValue());

            for (int n = 0; n < numSamples; ++n)
                x[n] = wdfCircuit[ch].processSample (x[n]);
        }
    }

    buffer.applyGain (Decibels::decibelsToGain (21.0f));
}
