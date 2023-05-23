#include "TrebleBooster.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

TrebleBooster::TrebleBooster (UndoManager* um) : BaseProcessor ("Treble Booster", createParameterLayout(), um)
{
    chowdsp::ParamUtils::loadParameterPointer (trebleParam, vts, "boost");

    uiOptions.backgroundColour = Colours::cyan.darker (0.15f);
    uiOptions.powerColour = Colours::red.darker (0.1f);
    uiOptions.info.description = "A treble boosting filter based on the tone circuit in the Klon Centaur distortion pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::treble_booster_schematic_svg,
                                               .size = BinaryData::treble_booster_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        1.8e3f,
        "R21",
        [this] (const netlist::CircuitQuantity& self)
        {
            R21 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        100.0e3f,
        "R22",
        [this] (const netlist::CircuitQuantity& self)
        {
            G1 = 1.0f / self.value.load();
        },
        10.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        4.7e3f,
        "R23",
        [this] (const netlist::CircuitQuantity& self)
        {
            R23 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        100.0e3f,
        "R24",
        [this] (const netlist::CircuitQuantity& self)
        {
            G4 = 1.0f / self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addCapacitor (
        3.9e-9f,
        "C14",
        [this] (const netlist::CircuitQuantity& self)
        {
            C = self.value.load();
        },
        50.0e-12f,
        1.0e-3f);
}

ParamLayout TrebleBooster::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "boost", "Boost", 0.25f);

    return { params.begin(), params.end() };
}

void TrebleBooster::prepare (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (samplesPerBlock);
    fs = (float) sampleRate;

    trebleSmooth.reset (sampleRate, 0.01);
    trebleSmooth.setCurrentAndTargetValue (*trebleParam);
    calcCoefs (trebleSmooth.getCurrentValue());

    for (auto& filt : iir)
        filt.reset();
}

void TrebleBooster::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();

    trebleSmooth.setTargetValue (*trebleParam);
    auto x = buffer.getArrayOfWritePointers();

    if (trebleSmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (trebleSmooth.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
            }
        }
        else if (numChannels == 2)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (trebleSmooth.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
                x[1][n] = iir[1].processSample (x[1][n]);
            }
        }
    }
    else
    {
        calcCoefs (trebleSmooth.getNextValue());
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                x[ch][n] = iir[ch].processSample (x[ch][n]);
        }
    }
}
