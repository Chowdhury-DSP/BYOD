#include "Flapjack.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

//https://aionfx.com/news/tracing-journal-crowther-hot-cake/
// Mode 1:
// - transistor is effectively bypassed
// - bias voltage = 4.9V
// Mode 2:
// - transistor is _active_
// - bias voltage = 4V

namespace FlapjackTags
{
const auto driveTag = "drive";
const auto presenceTag = "presence";
const auto levelTag = "level";
const auto modeTag = "mode";
const auto lowCutTag = "lowcut";
} // namespace

Flapjack::Flapjack (UndoManager* um)
    : BaseProcessor ("Flapjack", createParameterLayout(), um)
{
    using namespace ParameterHelpers;

    driveParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, FlapjackTags::driveTag));
    driveParam.setRampLength (0.025);
    driveParam.mappingFunction = [this] (float x)
    {
        if (magic_enum::enum_value<FlapjackClipMode> (modeParam->getIndex()) == FlapjackClipMode::LightFold)
            x *= 0.9f;
        return chowdsp::Power::ipow<2> (1.0f - x);
    };
    presenceParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, FlapjackTags::presenceTag));
    presenceParam.setRampLength (0.025);
    lowCutParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, FlapjackTags::lowCutTag));
    lowCutParam.setRampLength (0.025);

    loadParameterPointer (levelParam, vts, FlapjackTags::levelTag);
    loadParameterPointer (modeParam, vts, FlapjackTags::modeTag);

    uiOptions.backgroundColour = Colours::whitesmoke.darker (0.1f);
    uiOptions.powerColour = Colours::red.darker (0.2f);
    uiOptions.info.description = "Overdrive effect based on the \"Hot Cake\" overdrive pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::hot_cake_schematic_svg,
                                               .size = BinaryData::hot_cake_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        1.0e6f,
        "R1",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.Vb.setResistanceValue (self.value.load());
        },
        1.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R2.setResistanceValue (self.value.load());
        },
        100.0f,
        1.0e6f);
    netlistCircuitQuantities->addResistor (
        100.0e3f,
        "R3",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R3.setResistanceValue (self.value.load());
        },
        2.0e3f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R5",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R5.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        10.0e3f,
        "R6",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R6.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addResistor (
        1.0e3f,
        "R7",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.R7.setResistanceValue (self.value.load());
        },
        100.0f,
        2.0e6f);
    netlistCircuitQuantities->addCapacitor (
        10.0e-9f,
        "C1",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.Vin_C1.setCapacitanceValue (self.value.load());
        },
        100.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        10.0e-6f,
        "C2",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C2.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        22.0e-9f,
        "C6",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C6.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        1.0e-3f);
    netlistCircuitQuantities->addCapacitor (
        82.0e-9f,
        "C7",
        [this] (const netlist::CircuitQuantity& self)
        {
            for (auto& wdfModel : wdf)
                wdfModel.C7.setCapacitanceValue (self.value.load());
        },
        1.0e-12f,
        1.0e-3f);
}

ParamLayout Flapjack::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, FlapjackTags::driveTag, "Drive", 0.75f);
    createPercentParameter (params, FlapjackTags::presenceTag, "Presence", 0.5f);
    createFreqParameter (params, FlapjackTags::lowCutTag, "Low Cut", 20.0f, 750.0f, 100.0f, 100.0f);
    createPercentParameter (params, FlapjackTags::levelTag, "Level", 0.5f);
    emplace_param<chowdsp::ChoiceParameter> (params, FlapjackTags::modeTag, "Mode", StringArray { "Fizzy", "Bluesberry", "Peachy" }, 1);

    return { params.begin(), params.end() };
}

void Flapjack::prepare (double sampleRate, int samplesPerBlock)
{
    driveParam.prepare (sampleRate, samplesPerBlock);
    presenceParam.prepare (sampleRate, samplesPerBlock);
    lowCutParam.prepare (sampleRate, samplesPerBlock);

    for (auto& model : wdf)
        model.prepare (sampleRate);

    dcBlocker.prepare (2);
    dcBlocker.calcCoefs (20.0f, (float) sampleRate);

    level.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 });

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 12000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void Flapjack::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    driveParam.process (numSamples);
    presenceParam.process (numSamples);
    lowCutParam.process (numSamples);

    const auto clipMode = magic_enum::enum_value<FlapjackClipMode> (modeParam->getIndex());
    float makeupGainOffsetDB = 0.0f;
    magic_enum::enum_switch (
        [this, &buffer, makeupGainOffsetDB] (auto modeVal) mutable
        {
            static constexpr FlapjackClipMode mode = modeVal;
            if (driveParam.isSmoothing() || presenceParam.isSmoothing() || lowCutParam.isSmoothing())
            {
                const auto* driveSmooth = driveParam.getSmoothedBuffer();
                const auto* presenceSmooth = presenceParam.getSmoothedBuffer();
                const auto lowCutSmooth = lowCutParam.getSmoothedBuffer();
                for (auto [channelIndex, sampleIndex, subBlockData] : chowdsp::buffer_iters::sub_blocks<16> (buffer))
                {
                    wdf[channelIndex].setParams (driveSmooth[sampleIndex],
                                                 presenceSmooth[sampleIndex],
                                                 lowCutSmooth[sampleIndex]);
                    for (auto& x : subBlockData)
                        x = wdf[channelIndex].template processSample<mode> (x);
                }
            }
            else
            {
                for (auto [channelIndex, channelData] : chowdsp::buffer_iters::channels (buffer))
                {
                    wdf[channelIndex].setParams (driveParam.getCurrentValue(), presenceParam.getCurrentValue(), lowCutParam.getCurrentValue());
                    for (auto& x : channelData)
                        x = wdf[channelIndex].template processSample<mode> (x);
                }
            }

            if constexpr (mode == FlapjackClipMode::HardClip)
                makeupGainOffsetDB = 3.0f;
            else if constexpr (mode == FlapjackClipMode::AlgSigmoid)
                makeupGainOffsetDB = 4.5f;
            else if constexpr (mode == FlapjackClipMode::LightFold)
                makeupGainOffsetDB = 5.5f;
        },
        clipMode);

    const auto makeupGain = Decibels::decibelsToGain (-12.0f + makeupGainOffsetDB);
    level.setGainLinear (chowdsp::Power::ipow<2> (levelParam->getCurrentValue()) * -makeupGain);
    level.process (buffer);
    dcBlocker.processBlock (buffer);
}
