#include "MouseDrive.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

MouseDrive::MouseDrive (UndoManager* um) : BaseProcessor ("Mouse Drive", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (distortionParam, vts, "distortion");
    loadParameterPointer (volumeParam, vts, "volume");

    uiOptions.backgroundColour = Colours::wheat;
    uiOptions.info.description = "Virtual analog distortion effect based on the clipping stage from the ProCo RAT pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::proco_rat_schematic_svg,
                                               .size = BinaryData::proco_rat_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        1.0e6f,
        "R2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R2.setResistanceValue (self.value.load());
        },
        10.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R3",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R3.setResistanceValue (self.value.load());
        },
        100.0f,
        1.0e6f);
    netlistCircuitQuantities->addResistor (
        47.0f,
        "R4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R4_C5.setResistanceValue (self.value.load());
        },
        10.0f,
        10.0e3f);
    netlistCircuitQuantities->addResistor (
        560.0f,
        "R5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R5_C6.setResistanceValue (self.value.load());
        },
        10.0f,
        100.0e3f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R6",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R6_C7.setResistanceValue (self.value.load());
        },
        100.0f,
        1.0e6f);
    netlistCircuitQuantities->addCapacitor (
        22.0e-9f,
        "C1",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C1.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-9f,
        "C2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C2.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        1.0e-6f);
    netlistCircuitQuantities->addCapacitor (
        100.0e-12f,
        "C4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.Rd_C4.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        1.0e-6f);
    netlistCircuitQuantities->addCapacitor (
        2.2e-6f,
        "C5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R4_C5.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        4.7e-6f,
        "C6",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R5_C6.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        4.7e-6f,
        "C7",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R6_C7.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        1.0e-3f);
}

ParamLayout MouseDrive::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "distortion", "Distortion", 0.5f);
    createPercentParameter (params, "volume", "Volume", 0.5f);

    return { params.begin(), params.end() };
}

void MouseDrive::prepare (double sampleRate, int samplesPerBlock)
{
    for (auto& model : wdf)
        model.prepare (sampleRate);

    const auto spec = dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 2 };
    gain.setGainLinear (0.0f);
    gain.prepare (spec);
    gain.setRampDurationSeconds (0.05);

    dcBlocker.prepare (spec);
    dcBlocker.calcCoefs (15.0f, (float) sampleRate);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 20000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void MouseDrive::processAudio (AudioBuffer<float>& buffer)
{
    const auto RdVal = MouseDriveWDF::Rdistortion * std::pow (distortionParam->getCurrentValue(), 5.0f);
    for (auto [ch, data] : chowdsp::buffer_iters::channels (buffer))
    {
        wdf[ch].Rd_C4.setResistanceValue (RdVal);
        for (auto& x : data)
            x = wdf[ch].process (x);
    }

    const auto volumeParamVal = volumeParam->getCurrentValue();
    if (volumeParamVal < 0.01f)
        gain.setGainLinear (0.0f);
    else
        gain.setGainDecibels (-24.0f * (1.0f - volumeParamVal) - 12.0f);
    gain.process (buffer);

    dcBlocker.processBlock (buffer);
}
