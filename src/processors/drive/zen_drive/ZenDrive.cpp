#include "ZenDrive.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

ZenDrive::ZenDrive (UndoManager* um) : BaseProcessor ("Yen Drive", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (voiceParam, vts, "voice");
    loadParameterPointer (gainParam, vts, "gain");

    uiOptions.backgroundColour = Colours::cornsilk;
    uiOptions.info.description = "Virtual analog emulation of the ZenDrive overdrive pedal by Hermida Audio.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::zen_drive_schematic_svg,
                                               .size = BinaryData::zen_drive_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        470.0e3f,
        "R4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R4.setResistanceValue (self.value.load());
        },
        10.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addCapacitor (
        470.0e-9f,
        "C3",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C3.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        100.0e-12f,
        "C4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.Rv9_C4.setCapacitanceValue (self.value.load());
        },
        1.0e-15f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        100.0e-9f,
        "C5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R5_R6_C5.setCapacitanceValue (self.value.load());
        },
        1.0e-9f,
        1.0e-3f);
}

ParamLayout ZenDrive::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "voice", "Voice", 0.5f);
    createPercentParameter (params, "gain", "Gain", 0.5f);

    return { params.begin(), params.end() };
}

void ZenDrive::prepare (double sampleRate, int samplesPerBlock)
{
    for (auto& wdfProc : wdf)
    {
        wdfProc.prepare (sampleRate);
        wdfProc.setParameters (1.0f - *voiceParam, ParameterHelpers::logPot (*gainParam));
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 20000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void ZenDrive::processAudio (AudioBuffer<float>& buffer)
{
    buffer.applyGain (0.5f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParameters (1.0f - *voiceParam, ParameterHelpers::logPot (*gainParam));
        wdf[ch].process (buffer.getWritePointer (ch), buffer.getNumSamples());
    }

    dcBlocker.processAudio (buffer);

    buffer.applyGain (Decibels::decibelsToGain (-10.0f));
}
