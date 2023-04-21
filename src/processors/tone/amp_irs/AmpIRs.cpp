#include "AmpIRs.h"
#include "processors/ParameterHelpers.h"

AmpIRs::AmpIRs (UndoManager* um) : BaseProcessor ("Amp IRs", createParameterLayout(), um),
                                   convolution (getSharedConvolutionMessageQueue())
{
    audioFormatManager.registerBasicFormats();

    for (const auto& irName : StringArray (irNames.begin(), irNames.size() - 1))
    {
        auto binaryName = irName.replaceCharacter (' ', '_') + "_wav";
        int binarySize;
        auto* irData = BinaryData::getNamedResource (binaryName.getCharPointer(), binarySize);

        irMap.insert (std::make_pair (irName, std::make_pair ((void*) irData, (size_t) binarySize)));
    }

    using namespace ParameterHelpers;
    vts.addParameterListener (irTag, this);
    loadParameterPointer (mixParam, vts, mixTag);
    loadParameterPointer (gainParam, vts, gainTag);

    parameterChanged (irTag, vts.getRawParameterValue (irTag)->load());

    uiOptions.backgroundColour = Colours::darkcyan.brighter (0.1f);
    uiOptions.powerColour = Colours::red.brighter (0.0f);
    uiOptions.info.description = "A collection of impulse responses from guitar cabinets.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AmpIRs::~AmpIRs()
{
    vts.removeParameterListener (irTag, this);
}

ParamLayout AmpIRs::createParameterLayout()
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

void AmpIRs::setMakeupGain (float irSampleRate)
{
    makeupGainDB.store (Decibels::gainToDecibels (std::sqrt (irSampleRate / fs)));
}

void AmpIRs::parameterChanged (const String& parameterID, float newValue)
{
    if (parameterID != irTag)
        return;

    auto irIdx = (int) newValue;
    if (irIdx >= irNames.size() - 1)
        return;

    const auto& irName = irNames[irIdx];
    auto& irData = irMap[irName];
    irState.file = File {};
    irState.data = {};
    irState.paramIndex = irIdx;
    irState.name = irNames[irIdx];
    irChangedBroadcaster();

    setMakeupGain (96000.0f);

    ScopedLock sl (irMutex);
    convolution.loadImpulseResponse (irData.first, irData.second, dsp::Convolution::Stereo::yes, dsp::Convolution::Trim::yes, 0);
}

void AmpIRs::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    convolution.prepare (spec);

    gain.prepare (spec);
    gain.setRampDurationSeconds (0.01);

    dryWetMixer.prepare (spec);
    dryWetMixerMono.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });

    if (irState.paramIndex == customIRIndex)
        loadIRFromCurrentState();
    else
        parameterChanged (irTag, vts.getRawParameterValue (irTag)->load());
}

void AmpIRs::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    auto& dryWet = numChannels == 1 ? dryWetMixerMono : dryWetMixer;

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    dryWet.setWetMixProportion (mixParam->getCurrentValue());
    gain.setGainDecibels (gainParam->getCurrentValue() + makeupGainDB.load());

    dryWet.pushDrySamples (block);
    convolution.process (context);
    gain.process (context);
    dryWet.mixWetSamples (block);
}
