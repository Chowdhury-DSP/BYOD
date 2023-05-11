#include "TubeScreamerTone.h"
#include "../../ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

TubeScreamerTone::TubeScreamerTone (UndoManager* um) : BaseProcessor ("TS-Tone", createParameterLayout(), um)
{
    chowdsp::ParamUtils::loadParameterPointer (toneParam, vts, "tone");

    uiOptions.backgroundColour = Colours::limegreen.darker (0.1f);
    uiOptions.powerColour = Colours::silver.brighter (0.5f);
    uiOptions.info.description = "Virtual analog emulation of the Tube Screamer tone circuit.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::tube_screamer_tone_schematic_svg,
                                               .size = BinaryData::tube_screamer_tone_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R7",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.Vin_R7.setResistanceValue (self.value.load());
        },
        100.0f,
        100.0e3f);
    netlistCircuitQuantities->addResistor (
        220.0f,
        "R8",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R8_C6.setResistanceValue (self.value.load());
        },
        25.0f,
        10.0e3f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R9",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.Vplus_R9.setResistanceValue (self.value.load());
        },
        100.0f,
        1.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R11",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R11.setResistanceValue (self.value.load());
        },
        100.0f,
        100.0e3f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R12",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R12_C7.setResistanceValue (self.value.load());
        },
        100.0f,
        100.0e3f);
    netlistCircuitQuantities->addCapacitor (
        0.22e-6f,
        "C5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C5.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        20.0e-6f);
    netlistCircuitQuantities->addCapacitor (
        0.22e-6f,
        "C6",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R8_C6.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-6f,
        "C7",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R12_C7.setResistanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
}

ParamLayout TubeScreamerTone::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "tone", "Tone", 0.5f);

    return { params.begin(), params.end() };
}

void TubeScreamerTone::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    for (auto& w : wdf)
        w.prepare (sampleRate);

    for (auto& toneSm : toneSmooth)
        toneSm.reset (sampleRate, 0.02);
}

void TubeScreamerTone::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        toneSmooth[ch].setTargetValue (*toneParam);
        if (toneSmooth[ch].isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                wdf[ch].setParams (toneSmooth[ch].getNextValue());
                x[n] = wdf[ch].processSample (x[n]);
            }
        }
        else
        {
            wdf[ch].setParams (toneSmooth[ch].getNextValue());
            for (int n = 0; n < numSamples; ++n)
                x[n] = wdf[ch].processSample (x[n]);
        }
    }
}
