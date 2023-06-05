#include "BigMuffTone.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace
{
auto createComponentSets()
{
    // Component values taken from this blog post: https://www.coda-effects.com/p/big-muff-tonestack-dealing-with-mids.html
    return std::array<BigMuffTone::Components, 11> {
        BigMuffTone::Components { .name = "Triangle", .R8 = 22.0e3f, .C8 = 10.0e-9f, .C9 = 4.0e-9f, .R5 = 22.0e3f },
        BigMuffTone::Components { .name = "Ram's Head '73", .R8 = 33.0e3f, .C8 = 10.0e-9f, .C9 = 4.0e-9f, .R5 = 33.0e3f },
        BigMuffTone::Components { .name = "Ram's Head '75", .R8 = 39.0e3f, .C8 = 10.0e-9f, .C9 = 4.0e-9f, .R5 = 22.0e3f },
        BigMuffTone::Components { .name = "Russian", .R8 = 20.0e3f, .C8 = 10.0e-9f, .C9 = 3.9e-9f, .R5 = 22.0e3f },
        BigMuffTone::Components { .name = "Hoof Fuzz", .R8 = 39.0e3f, .C8 = 6.8e-9f, .C9 = 6.8e-9f, .R5_1 = 2.2e3f, .R5_2 = 25.0e3f },
        BigMuffTone::Components { .name = "AMZ v1", .R8 = 39.0e3f, .C8 = 10.0e-9f, .C9 = 12.0e-9f, .R5_1 = 3.3e3f, .R5_2 = 25.0e3f },
        BigMuffTone::Components { .name = "AMZ v2", .R8 = 470.0e3f, .C8 = 1.5e-9f, .C9 = 15.0e-9f, .R5_1 = 3.3e3f, .R5_2 = 25.0e3f, .RT = 250.0e3f },
        BigMuffTone::Components { .name = "Supercollider", .R8 = 20.0e3f, .C8 = 10.0e-9f, .C9 = 3.9e-9f, .R5_1 = 10.0e3f, .R5_2 = 25.0e3f },
        BigMuffTone::Components { .name = "Flat Mids", .R8 = 39.0e3f, .C8 = 10.0e-9f, .C9 = 10.0e-9f, .R5 = 22.0e3f },
        BigMuffTone::Components { .name = "Bump Mids", .R8 = 39.0e3f, .C8 = 5.6e-9f, .C9 = 10.0e-9f, .R5 = 22.0e3f },
        BigMuffTone::Components { .name = "Custom", .R8 = 39.0e3f, .C8 = 10.0e-9f, .C9 = 4.0e-9f, .R5_1 = 7.7e3f, .R5_2 = 28.6e3f, .RT = 100.0e3f },
    };
}
} // namespace

BigMuffTone::BigMuffTone (UndoManager* um) : BaseProcessor ("Muff Tone", createParameterLayout(), um),
                                             componentSets (createComponentSets())
{
    using namespace ParameterHelpers;
    loadParameterPointer (toneParam, vts, "tone");
    loadParameterPointer (midsParam, vts, "mids");
    typeParam = vts.getRawParameterValue ("type");

    uiOptions.backgroundColour = Colours::darkred.brighter (0.3f);
    uiOptions.powerColour = Colours::lime.darker (0.1f);
    uiOptions.info.description = "Emulation of the tone stage from various Big Muff-style pedals.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::muff_tone_schematic_svg,
                                               .size = BinaryData::muff_tone_schematic_svgSize };
    netlistCircuitQuantities->extraNote = R"(To activate the netlist circuit quantities, choose the "Custom" option for the "Type" parameter.)";

    netlistCircuitQuantities->addResistor (
        7.7e3f,
        "R5",
        [this] (const netlist::CircuitQuantity& self)
        {
            componentSets.back().R5_1 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        28.6e3f,
        "R_mids",
        [this] (const netlist::CircuitQuantity& self)
        {
            componentSets.back().R5_2 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        39.0e3f,
        "R8",
        [this] (const netlist::CircuitQuantity& self)
        {
            componentSets.back().R8 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        100.0e3f,
        "R_tone",
        [this] (const netlist::CircuitQuantity& self)
        {
            componentSets.back().RT = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addCapacitor (
        10.0e-9f,
        "C8",
        [this] (const netlist::CircuitQuantity& self)
        {
            componentSets.back().C8 = self.value.load();
        },
        25.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        4.0e-9f,
        "C9",
        [this] (const netlist::CircuitQuantity& self)
        {
            componentSets.back().C9 = self.value.load();
        },
        25.0e-12f,
        1.0e-3f);
}

ParamLayout BigMuffTone::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "tone", "Tone", 0.5f);
    createPercentParameter (params, "mids", "Mids", 0.5f);

    StringArray types;
    types.ensureStorageAllocated ((int) numComponentSets);
    for (const auto& set : createComponentSets())
        types.add (set.name.data());
    emplace_param<AudioParameterChoice> (params, "type", "Type", types, 2);

    return { params.begin(), params.end() };
}

void BigMuffTone::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    fs = (float) sampleRate;

    const auto& comps = componentSets[(int) *typeParam];
    toneSmooth.reset (sampleRate, 0.01);
    toneSmooth.setCurrentAndTargetValue (*toneParam);
    midsSmooth.reset (sampleRate, 0.01);
    midsSmooth.setCurrentAndTargetValue (*midsParam);
    calcCoefs (toneSmooth.getNextValue(), midsSmooth.getNextValue(), comps);

    for (auto& filt : iir)
        filt.reset();
}

void BigMuffTone::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    const auto& comps = componentSets[(int) *typeParam];
    toneSmooth.setTargetValue (*toneParam);
    midsSmooth.setTargetValue (*midsParam);
    auto x = buffer.getArrayOfWritePointers();

    if (toneSmooth.isSmoothing() || midsSmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                calcCoefs (toneSmooth.getNextValue(), midsSmooth.getNextValue(), comps);
                x[0][n] = iir[0].processSample (x[0][n]);
            }
        }
        else if (numChannels == 2)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                calcCoefs (toneSmooth.getNextValue(), midsSmooth.getNextValue(), comps);
                x[0][n] = iir[0].processSample (x[0][n]);
                x[1][n] = iir[1].processSample (x[1][n]);
            }
        }
    }
    else
    {
        calcCoefs (toneSmooth.getNextValue(), midsSmooth.getNextValue(), comps);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < numSamples; ++n)
                x[ch][n] = iir[ch].processSample (x[ch][n]);
        }
    }

    buffer.applyGain (Decibels::decibelsToGain (6.0f));
}
