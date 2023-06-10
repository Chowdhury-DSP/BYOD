#include "Centaur.h"
#include "processors/netlist_helpers/NetlistViewer.h"

namespace
{
const String gainTag = "gain";
const String levelTag = "level";
const String modeTag = "mode";
} // namespace

Centaur::Centaur (UndoManager* um) : BaseProcessor ("Centaur", createParameterLayout(), um),
                                     gainStageML (vts)
{
    using namespace ParameterHelpers;
    loadParameterPointer (gainParam, vts, gainTag);
    loadParameterPointer (levelParam, vts, levelTag);
    modeParam = vts.getRawParameterValue (modeTag);
    addPopupMenuParameter (modeTag);

    uiOptions.backgroundColour = Colour (0xFFDAA520);
    uiOptions.powerColour = Colour (0xFF14CBF2).brighter (0.5f);
    uiOptions.info.description = "Emulation of the Klon Centaur overdrive pedal. Use the right-click menu to enable neural mode.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    uiOptions.info.infoLink = "https://github.com/jatinchowdhury18/KlonCentaur";

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::centaur_schematic_svg,
                                               .size = BinaryData::centaur_schematic_svgSize };
    netlistCircuitQuantities->extraNote = "Most circuit quantities will only affect the audio signal when the processor is in \"Traditional\" mode.";
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R1",
        [this] (const netlist::CircuitQuantity& self)
        {
            inputBuffer.R1 = self.value.load();
            inputBuffer.calc_coefs();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e6f,
        "R2",
        [this] (const netlist::CircuitQuantity& self)
        {
            inputBuffer.R2 = self.value.load();
            inputBuffer.calc_coefs();
        },
        1.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R5_C4.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R6",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.preamp_wdf)
                wdf.R6.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.5e3f,
        "R7",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.preamp_wdf)
                wdf.R7.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.5e3f,
        "R8",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R8.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R9",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R9_C6.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        2.0e3f,
        "R10",
        [this] (const netlist::CircuitQuantity& self)
        {
            gainStage.amp_stage.R10 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        15.0e3f,
        "R11",
        [this] (const netlist::CircuitQuantity& self)
        {
            gainStage.amp_stage.R11 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        422.0e3f,
        "R12",
        [this] (const netlist::CircuitQuantity& self)
        {
            gainStage.amp_stage.R12 = self.value.load();
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R13",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.clipping_wdf)
                wdf.R13.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        22.0e3f,
        "R15",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R15_C11.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        47.0e3f,
        "R16",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.clipping_wdf)
                wdf.Vbias.setResistanceValue (self.value.load());
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R16.setResistanceValue (self.value.load());
        },
        20.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        12.0e3f,
        "R18",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R18_C12.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        15.0e3f,
        "R19",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.preamp_wdf)
                wdf.Vbias2.setResistanceValue (self.value.load());
        },
        1.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        392.0e3f,
        "R20",
        [this] (const netlist::CircuitQuantity& self)
        {
            gainStage.summing_amp.R20 = self.value.load();
            gainStage.summing_amp.calc_coefs();
        },
        10.0e3f,
        1.0e6f);
    netlistCircuitQuantities->addCapacitor (
        0.1e-6f,
        "C1",
        [this] (const netlist::CircuitQuantity& self)
        {
            inputBuffer.C1 = self.value.load();
            inputBuffer.calc_coefs();
        },
        10.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        0.1e-6f,
        "C3",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.preamp_wdf)
                wdf.C3.setCapacitanceValue (self.value.load());
        },
        1.0e-9f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        68.0e-9f,
        "C4",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R5_C4.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        68.0e-9f,
        "C5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.preamp_wdf)
                wdf.C5.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        390.0e-9f,
        "C6",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R9_C6.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        82.0e-9f,
        "C7",
        [this] (const netlist::CircuitQuantity& self)
        {
            gainStage.amp_stage.C7 = self.value.load();
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        390.0e-12f,
        "C8",
        [this] (const netlist::CircuitQuantity& self)
        {
            gainStage.amp_stage.C8 = self.value.load();
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-6f,
        "C9",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.clipping_wdf)
                wdf.C9.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-6f,
        "C10",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.clipping_wdf)
                wdf.C10.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        2.2e-9f,
        "C11",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R15_C11.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        27.0e-9f,
        "C12",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.ff2_wdf)
                wdf.R18_C12.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        820.0e-12f,
        "C13",
        [this] (const netlist::CircuitQuantity& self)
        {
            gainStage.summing_amp.C13 = self.value.load();
            gainStage.summing_amp.calc_coefs();
        },
        1.0e-12f,
        100.0e-9f);
    netlistCircuitQuantities->addCapacitor (
        1.0e-6f,
        "C16",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdf : gainStage.preamp_wdf)
                wdf.C16.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        100.0e-3f);
}

ParamLayout Centaur::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, gainTag, "Gain", 0.5f);
    createPercentParameter (params, levelTag, "Level", 0.5f);
    emplace_param<AudioParameterChoice> (params, modeTag, "Mode", StringArray { "Traditional", "Neural" }, 0);

    return { params.begin(), params.end() };
}

void Centaur::prepare (double sampleRate, int samplesPerBlock)
{
    gainStage.prepare (sampleRate, samplesPerBlock, 2);
    gainStageML.reset (sampleRate, samplesPerBlock);

    inputBuffer.prepare ((float) sampleRate, 2);
    outputStage.prepare ((float) sampleRate, samplesPerBlock, 2);

    fadeBuffer.setSize (2, samplesPerBlock);

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 10000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void Centaur::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    chowdsp::BufferMath::applyGain (buffer, 0.5f);
    inputBuffer.processBlock (buffer);

    for (auto [_, data] : chowdsp::buffer_iters::channels (buffer))
        juce::FloatVectorOperations::clip (data.data(), data.data(), -4.5f, 4.5f, data.size());

    const bool useML = *modeParam == 1.0f;
    if (useML == useMLPrev)
    {
        if (useML) // use rnn
            gainStageML.processBlock (buffer);
        else // use circuit model
            gainStage.process (buffer, gainParam->getCurrentValue());
    }
    else
    {
        fadeBuffer.makeCopyOf (buffer, true);

        if (useML) // use rnn
        {
            gainStageML.processBlock (buffer);
            gainStage.process (fadeBuffer, gainParam->getCurrentValue());
        }
        else // use circuit model
        {
            gainStage.process (buffer, gainParam->getCurrentValue());
            gainStageML.processBlock (fadeBuffer);
        }

        buffer.applyGainRamp (0, numSamples, 0.0f, 1.0f);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFromWithRamp (ch, 0, fadeBuffer.getReadPointer (ch), numSamples, 1.0f, 0.0f);

        useMLPrev = useML;
    }

    outputStage.process_block (buffer, *levelParam);
    dcBlocker.processAudio (buffer);
}
