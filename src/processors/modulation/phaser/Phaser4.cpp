#include "Phaser4.h"
#include "processors/BufferHelpers.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace Phaser4Tags
{
const String rateTag = "rate";
const String depthTag = "depth";
const String feedbackTag = "feedback";
const String fbStageTag = "fb_stage";
const String stereoTag = "stereo";
const String mixTag = "mix";
} // namespace Phaser4Tags

Phaser4::Phaser4 (UndoManager* um) : BaseProcessor (
    "Phaser4",
    createParameterLayout(),
    InputPort {},
    OutputPort {},
    um,
    [] (InputPort port)
    {
        if (port == InputPort::ModulationInput)
            return PortType::modulation;
        return PortType::audio;
    },
    [] (OutputPort port)
    {
        if (port == OutputPort::ModulationOutput)
            return PortType::modulation;
        return PortType::audio;
    })
{
    using namespace ParameterHelpers;
    loadParameterPointer (rateHzParam, vts, Phaser4Tags::rateTag);
    loadParameterPointer (fbStageParam, vts, Phaser4Tags::fbStageTag);
    loadParameterPointer (stereoParam, vts, Phaser4Tags::stereoTag);

    depthParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, Phaser4Tags::depthTag));
    depthParam.mappingFunction = [] (float x)
    { return 0.45f * x; };

    feedbackParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, Phaser4Tags::feedbackTag));
    feedbackParam.mappingFunction = [] (float x)
    { return 0.95f * x; };

    const auto* mixParam = getParameterPointer<chowdsp::FloatParameter*> (vts, Phaser4Tags::mixTag);
    dryMix.setParameterHandle (mixParam);
    dryMix.setRampLength (0.05);
    dryMix.mappingFunction = [] (float x)
    { return 1.0f - x; };
    wetMix.setParameterHandle (mixParam);
    wetMix.setRampLength (0.05);

    lfoShaper.initialise ([] (float x)
                          {
                              static constexpr auto skewFactor = gcem::pow (2.0f, -0.5f);
                              return 2.0f * std::pow ((x + 1.0f) * 0.5f, skewFactor) - 1.0f; },
                          -1.0f,
                          1.0f,
                          2048);

    addPopupMenuParameter (Phaser4Tags::stereoTag);
    disableWhenInputConnected ({ Phaser4Tags::rateTag }, ModulationInput);

    uiOptions.backgroundColour = Colour { 0xfffc7533 };
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "A phaser effect based on a classic 4-stage phaser pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    netlistCircuitQuantities = std::make_unique<netlist::CircuitQuantityList>();
    netlistCircuitQuantities->schematicSVG = { .data = BinaryData::phase90_schematic_svg,
                                               .size = BinaryData::phase90_schematic_svgSize };
    netlistCircuitQuantities->addResistor (
        24.0e3f,
        "R6",
        [this] (const netlist::CircuitQuantity& self)
        {
            const auto newValue = self.value.load();
            for (int ch = 0; ch < 2; ++ch)
            {
                fb4Filter[ch].setResistor (newValue);
                fb3Filter[ch].setResistor (newValue);
                fb2Filter[ch].setResistor (newValue);
            }
        },
        100.0f,
        40.0e3f);
    netlistCircuitQuantities->addCapacitor (
        47.0e-9f,
        "C1",
        [this] (const netlist::CircuitQuantity& self)
        {
            const auto newValue = self.value.load();
            for (int ch = 0; ch < 2; ++ch)
            {
                fb4Filter[ch].setCapacitor (newValue);
                fb3Filter[ch].setCapacitor (newValue);
                fb2Filter[ch].setCapacitor (newValue);
            }
        },
        1.0e-12f,
        80.0e-9f);
}

ParamLayout Phaser4::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createFreqParameter (params, Phaser4Tags::rateTag, "Rate", 0.1f, 10.0f, 1.0f, 1.0f);
    createPercentParameter (params, Phaser4Tags::depthTag, "Depth", 1.0f);
    createBipolarPercentParameter (params, Phaser4Tags::feedbackTag, "Feedback", 0.6f);
    createPercentParameter (params, Phaser4Tags::mixTag, "Mix", 0.5f);
    emplace_param<chowdsp::ChoiceParameter> (params,
                                             Phaser4Tags::fbStageTag,
                                             "FB Stage",
                                             StringArray { "2nd Stage", "3rd Stage", "4th Stage" },
                                             0);
    emplace_param<chowdsp::BoolParameter> (params, Phaser4Tags::stereoTag, "Stereo", false);

    return { params.begin(), params.end() };
}

void Phaser4::prepare (double sampleRate, int samplesPerBlock)
{
    depthParam.prepare (sampleRate, samplesPerBlock);
    depthParam.setRampLength (0.05);
    feedbackParam.prepare (sampleRate, samplesPerBlock);
    feedbackParam.setRampLength (0.05);

    triangleLfo.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 1 });
    modData.resize ((size_t) samplesPerBlock, 0.0f);

    for (int ch = 0; ch < 2; ++ch)
    {
        fb4Filter[ch].prepare ((float) sampleRate);
        fb3Filter[ch].prepare ((float) sampleRate);
        fb2Filter[ch].prepare ((float) sampleRate);
    }

    dryMix.prepare (sampleRate, samplesPerBlock);
    wetMix.prepare (sampleRate, samplesPerBlock);

    audioOutBuffer.setSize (2, samplesPerBlock);
    modOutBuffer.setSize (1, samplesPerBlock);
}

void Phaser4::processModulation (int numSamples)
{
    modOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (ModulationInput))
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
    }
    else
    {
        triangleLfo.setFrequency (rateHzParam->getCurrentValue());
        modOutBuffer.clear();
        triangleLfo.processBlock (modOutBuffer);

        lfoShaper.process (modOutBuffer.getReadPointer (0),
                           modOutBuffer.getWritePointer (0),
                           numSamples);
    }

    FloatVectorOperations::multiply (modData.data(),
                                     modOutBuffer.getReadPointer (0),
                                     depthParam.getSmoothedBuffer(),
                                     numSamples);
    FloatVectorOperations::add (modData.data(), 0.5f, numSamples);
}

void Phaser4::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    depthParam.process (numSamples);
    feedbackParam.process (numSamples);
    dryMix.process (numSamples);
    wetMix.process (numSamples);
    processModulation (numSamples);

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numInChannels = audioInBuffer.getNumChannels();

        const auto stereoMode = stereoParam->get();
        const auto numOutChannels = stereoMode ? 2 : numInChannels;
        audioOutBuffer.setSize (numOutChannels, numSamples, false, false, true);

        const auto* fbData = feedbackParam.getSmoothedBuffer();
        auto* modDataPtr = modData.data();

        const auto fbStage = fbStageParam->getIndex() + 2;
        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            if (stereoMode && ch == 1)
            {
                FloatVectorOperations::negate (modDataPtr, modDataPtr, numSamples);
                FloatVectorOperations::add (modDataPtr, 1.0f, numSamples);
            }

            const auto* xIn = audioInBuffer.getReadPointer (ch % numInChannels);
            auto* xOut = audioOutBuffer.getWritePointer (ch);

            if (fbStage == 2)
                fb2Filter[ch].processBlock (xIn, xOut, modDataPtr, fbData, numSamples);
            else if (fbStage == 3)
                fb3Filter[ch].processBlock (xIn, xOut, modDataPtr, fbData, numSamples);
            else if (fbStage == 4)
                fb4Filter[ch].processBlock (xIn, xOut, modDataPtr, fbData, numSamples);

            juce::FloatVectorOperations::multiply (xOut, wetMix.getSmoothedBuffer(), numSamples);
            juce::FloatVectorOperations::addWithMultiply (xOut, xIn, dryMix.getSmoothedBuffer(), numSamples);
        }
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}

void Phaser4::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    modOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
    }
    else
    {
        modOutBuffer.clear();
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);
        audioOutBuffer.makeCopyOf (audioInBuffer, true);
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}

void Phaser4::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);

    using namespace std::string_view_literals;
    if (version <= chowdsp::Version { "1.2.0"sv })
    {
        // The "Mix" control was only introduced in version 1.2.1. Prior to that, mix was always at 100% wet.
        auto* mixParam = vts.getParameter (Phaser4Tags::mixTag);
        mixParam->setValueNotifyingHost (1.0f);
    }
}
