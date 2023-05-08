#include "TubeScreamer.h"
#include "../diode_circuits/DiodeParameter.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

TubeScreamer::TubeScreamer (UndoManager* um)
    : BaseProcessor ("Tube Screamer", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (gainParam, vts, "gain");
    diodeTypeParam = vts.getRawParameterValue ("diode");
    loadParameterPointer (nDiodesParam, vts, "num_diodes");

    uiOptions.backgroundColour = Colours::tomato.darker (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter (0.2f);
    uiOptions.info.description = "Virtual analog emulation of the clipping stage from the Tube Screamer overdrive pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::tube_screamer_schematic_svg,
                                               .size = BinaryData::tube_screamer_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        4.7e3f,
        "R4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R4.setResistanceValue (self.value.load());
        },
        100.0f,
        25.0e3f);
    netlistCircuitQuantities->addResistor (10.0e3f,
                                           "R5",
                                           [this] (const netlist::CircuitQuantity& self)
                                           {
                                               for (auto& wdfModel : wdf)
                                                   wdfModel.R5.setResistanceValue (self.value.load());
                                           });
    netlistCircuitQuantities->addCapacitor (
        1.0e-6f,
        "C2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C2.setCapacitanceValue (self.value.load());
        },
        100.0e-12f);
    netlistCircuitQuantities->addCapacitor (
        0.047e-6f,
        "C3",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C3.setCapacitanceValue (self.value.load());
        },
        1.0e-9f);
    netlistCircuitQuantities->addCapacitor (51.0e-12f,
                                            "C4",
                                            [this] (const netlist::CircuitQuantity& self)
                                            {
                                                for (auto& wdfModel : wdf)
                                                    wdfModel.C4.setCapacitanceValue (self.value.load());
                                            });
}

ParamLayout TubeScreamer::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "gain", "Gain", 0.5f);
    DiodeParameter::createDiodeParam (params, "diode");
    DiodeParameter::createNDiodesParam (params, "num_diodes");

    return { params.begin(), params.end() };
}

void TubeScreamer::prepare (double sampleRate, int samplesPerBlock)
{
    int diodeType = static_cast<int> (*diodeTypeParam);
    auto gainParamSkew = ParameterHelpers::logPot (*gainParam);
    for (auto& wdfProc : wdf)
    {
        wdfProc.prepare (sampleRate);
        wdfProc.setParameters (gainParamSkew, DiodeParameter::getDiodeIs (diodeType), *nDiodesParam, true);
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 10000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void TubeScreamer::processAudio (AudioBuffer<float>& buffer)
{
    buffer.applyGain (Decibels::decibelsToGain (-6.0f));

    int diodeType = static_cast<int> (*diodeTypeParam);
    auto gainParamSkew = ParameterHelpers::logPot (*gainParam);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParameters (gainParamSkew, DiodeParameter::getDiodeIs (diodeType), *nDiodesParam);
        wdf[ch].process (buffer.getWritePointer (ch), buffer.getNumSamples());
    }

    dcBlocker.processAudio (buffer);

    buffer.applyGain (Decibels::decibelsToGain (-6.0f));
}
