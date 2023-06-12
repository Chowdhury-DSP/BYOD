#include "CryBaby.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace
{
const String controlFreqTag = "control_freq";
}

CryBaby::CryBaby (UndoManager* um)
    : BaseProcessor ("Crying Child", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (controlFreqParam, vts, controlFreqTag);

    vr1Smooth.setRampLength (0.01);
    vr1Smooth.mappingFunction = [] (float x)
    { return 0.64f * std::pow (x, 0.5f); };

    uiOptions.backgroundColour = Colours::whitesmoke.darker (0.1f);
    uiOptions.powerColour = Colours::red.darker (0.2f);
    uiOptions.info.description = "\"Wah\" effect based on the Dunlop Cry Baby pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::cry_baby_schematic_svg,
                                               .size = BinaryData::cry_baby_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        68.0e3f,
        "R1",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.Vin_R1_C1.setResistanceValue (self.value.load());
        },
        2.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.5e3f,
        "R2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.R2.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        22.0e3f,
        "R3",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.R3.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        470.0e3f,
        "R5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.outputStage.R5.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        470.0e3f,
        "R6",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.R6.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        33.0e3f,
        "R7",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.R7.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        82.0e3f,
        "R8",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.R8_C3.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addCapacitor (
        0.01e-6f,
        "C1",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.Vin_R1_C1.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        0.1e-6f,
        "C2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.C2_Vfb.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        4.7e-6f,
        "C3",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.R8_C3.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        0.22e-6f,
        "C4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.outputStage.C4.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        0.22e-6f,
        "C5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.outputStage.C5.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addInductor (
        1.0f,
        "L1",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.activeStage.L1.setInductanceValue (self.value.load());
        });
}

ParamLayout CryBaby::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, controlFreqTag, "Freq", 0.5f);
    return { params.begin(), params.end() };
}

void CryBaby::prepare (double sampleRate, int samplesPerBlock)
{
    vr1Smooth.prepare (sampleRate, samplesPerBlock);

    for (auto& wdfModel : wdf)
        wdfModel.prepare ((float) sampleRate);
}

void CryBaby::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    chowdsp::BufferMath::applyGain (buffer, Decibels::decibelsToGain (37.0f));

    vr1Smooth.process (controlFreqParam->getCurrentValue(), numSamples);
    //    if (vr1Smooth.isSmoothing())
    {
        const auto* vr1SmoothData = vr1Smooth.getSmoothedBuffer();
        for (auto [ch, channelData] : chowdsp::buffer_iters::channels (buffer))
        {
            for (auto [n, sample] : chowdsp::enumerate (channelData))
            {
                wdf[ch].setWahAmount (vr1SmoothData[n]);
                sample = wdf[ch].processSample (sample);
            }
        }
    }
    //    else
    //    {
    //        for (auto [ch, channelData] : chowdsp::buffer_iters::channels (buffer))
    //        {
    //            wdf[ch].setWahAmount (vr1Smooth.getCurrentValue());
    //            for (auto& sample : channelData)
    //                sample = wdf[ch].processSample (sample);
    //        }
    //    }
}
