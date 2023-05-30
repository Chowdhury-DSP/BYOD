#include "BassCleaner.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

BassCleaner::BassCleaner (UndoManager* um) : BaseProcessor ("Bass Cleaner", createParameterLayout(), um)
{
    chowdsp::ParamUtils::loadParameterPointer (cleanParam, vts, "clean");

    uiOptions.backgroundColour = Colours::dodgerblue.darker();
    uiOptions.powerColour = Colours::red.brighter (0.1f);
    uiOptions.info.description = "A filter to smooth and dampen bass frequencies.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::bass_cleaner_schematic_svg,
                                               .size = BinaryData::bass_cleaner_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        3.3e3f,
        "R4",
        [this] (const netlist::CircuitQuantity& self)
        {
            R4 = self.value.load();
        },
        100.0f,
        100.0e3f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-6f,
        "C3",
        [this] (const netlist::CircuitQuantity& self)
        {
            C3 = self.value.load();
        },
        1.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        47.0e-9f,
        "C4",
        [this] (const netlist::CircuitQuantity& self)
        {
            C4 = self.value.load();
        },
        1.0e-12f,
        10.0e-6f);
}

ParamLayout BassCleaner::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "clean", "Clean", 0.5f);

    return { params.begin(), params.end() };
}

void BassCleaner::prepare (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (samplesPerBlock);
    fs = (float) sampleRate;

    Rv1.reset (sampleRate, 0.05);
    Rv1.setCurrentAndTargetValue (ParameterHelpers::logPot (*cleanParam) * Rv1Value);
    calcCoefs (Rv1.getCurrentValue());

    for (auto& filt : iir)
        filt.reset();
}

void BassCleaner::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();

    Rv1.setTargetValue (ParameterHelpers::logPot (*cleanParam) * Rv1Value);
    auto x = buffer.getArrayOfWritePointers();

    if (Rv1.isSmoothing())
    {
        if (numChannels == 1)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (Rv1.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
            }
        }
        else if (numChannels == 2)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                calcCoefs (Rv1.getNextValue());
                x[0][n] = iir[0].processSample (x[0][n]);
                x[1][n] = iir[1].processSample (x[1][n]);
            }
        }
    }
    else
    {
        calcCoefs (Rv1.getNextValue());
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                x[ch][n] = iir[ch].processSample (x[ch][n]);
        }
    }
}
