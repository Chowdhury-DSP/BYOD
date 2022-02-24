#include "LofiIrs.h"
#include "../ParameterHelpers.h"

namespace
{
const StringArray irNames {
    "Casio 1",
    "Casio 2",
    "Cassette Speaker",
    "Toy Circuits",
    "Toy Mic",
    "Toy Speaker",
    "Yamaha 1",
    "Yamaha 2",
    "Yamaha 3",
    "Yamaha 4",
};

const String irTag = "ir";
const String mixTag = "mix";
const String gainTag = "gain";
} // namespace

LofiIrs::LofiIrs (UndoManager* um) : BaseProcessor ("LoFi IRs", createParameterLayout(), um),
                                     convolution (getSharedConvolutionMessageQueue())
{
    for (const auto& irName : irNames)
    {
        auto binaryName = irName.replaceCharacter (' ', '_') + "_wav";
        int binarySize;
        auto* irData = BinaryData::getNamedResource (binaryName.getCharPointer(), binarySize);

        irMap.insert (std::make_pair (irName, std::make_pair ((void*) irData, (size_t) binarySize)));
    }

    vts.addParameterListener (irTag, this);
    mixParam = vts.getRawParameterValue (mixTag);
    gainParam = vts.getRawParameterValue (gainTag);

    uiOptions.backgroundColour = Colours::darkgrey.brighter (0.15f);
    uiOptions.powerColour = Colours::red.darker (0.1f);
    uiOptions.info.description = "A collection of impulse responses from vintage toys and keyboards.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

LofiIrs::~LofiIrs()
{
    vts.removeParameterListener (irTag, this);
}

AudioProcessorValueTreeState::ParameterLayout LofiIrs::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    params.push_back (std::make_unique<AudioParameterChoice> (irTag,
                                                              "IR",
                                                              irNames,
                                                              0));

    createGainDBParameter (params, gainTag, "Gain", -18.0f, 18.0f, 0.0f);
    createPercentParameter (params, mixTag, "Mix", 1.0f);

    return { params.begin(), params.end() };
}

void LofiIrs::parameterChanged (const String& parameterID, float newValue)
{
    if (parameterID != irTag)
        return;

    auto irIdx = (int) newValue;
    auto& irData = irMap[irNames[irIdx]];
    convolution.loadImpulseResponse (irData.first, irData.second, dsp::Convolution::Stereo::yes, dsp::Convolution::Trim::yes, 0);
}

void LofiIrs::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    convolution.prepare (spec);
    parameterChanged (irTag, vts.getRawParameterValue (irTag)->load());

    gain.prepare (spec);
    gain.setRampDurationSeconds (0.01);

    dryWetMixer.prepare (spec);
    dryWetMixerMono.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });
}

void LofiIrs::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    auto& dryWet = numChannels == 1 ? dryWetMixerMono : dryWetMixer;

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    dryWet.setWetMixProportion (mixParam->load());
    gain.setGainDecibels (gainParam->load());

    dryWet.pushDrySamples (block);
    convolution.process (context);
    gain.process (context);
    dryWet.mixWetSamples (block);
}
