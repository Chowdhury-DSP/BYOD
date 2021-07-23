#include "ProcessorChain.h"
#include "ParameterHelpers.h"
#include "ProcessorChainActions.h"

namespace
{
const String monoModeTag = "mono_stereo";
const String oversamplingTag = "oversampling";
const String inGainTag = "in_gain";
const String outGainTag = "out_gain";
const String dryWetTag = "dry_wet";
} // namespace

ProcessorChain::ProcessorChain (ProcessorStore& store, AudioProcessorValueTreeState& vts) : procStore (store),
                                                                                            um (vts.undoManager)
{
    using namespace std::placeholders;
    procStore.addProcessorCallback = std::bind (&ProcessorChain::addProcessor, this, _1);

    monoModeParam = vts.getRawParameterValue (monoModeTag);
    oversamplingParam = vts.getRawParameterValue (oversamplingTag);
    inGainParam = vts.getRawParameterValue (inGainTag);
    outGainParam = vts.getRawParameterValue (outGainTag);
    dryWetParam = vts.getRawParameterValue (dryWetTag);

    for (int i = 0; i < 5; ++i)
        overSample[i] = std::make_unique<dsp::Oversampling<float>> (2, i, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
}

void ProcessorChain::createParameters (Parameters& params)
{
    params.push_back (std::make_unique<AudioParameterChoice> (monoModeTag,
                                                              "Mono/Stereo",
                                                              StringArray { "Mono", "Stereo" },
                                                              0));

    params.push_back (std::make_unique<AudioParameterChoice> (oversamplingTag,
                                                              "Oversampling",
                                                              StringArray { "1x", "2x", "4x", "8x", "16x" },
                                                              1));

    using namespace ParameterHelpers;
    createGainDBParameter (params, inGainTag, "In Gain", -72.0f, 18.0f, 0.0f);
    createGainDBParameter (params, outGainTag, "Out Gain", -72.0f, 18.0f, 0.0f);
    createPercentParameter (params, dryWetTag, "Dry/Wet", 1.0f);
}

void ProcessorChain::initializeProcessors (int curOS)
{
    const auto osFactor = (int) overSample[curOS]->getOversamplingFactor();
    const double osSampleRate = mySampleRate * osFactor;
    const int osSamplesPerBlock = mySamplesPerBlock * osFactor;

    for (auto* processor : procs)
        processor->prepare (osSampleRate, osSamplesPerBlock);
}

void ProcessorChain::prepare (double sampleRate, int samplesPerBlock)
{
    mySampleRate = sampleRate;
    mySamplesPerBlock = samplesPerBlock;

    monoBuffer.setSize (1, samplesPerBlock * 16); // allocate extra space for upsampled buffers
    stereoBuffer.setSize (2, samplesPerBlock * 16);

    int curOS = static_cast<int> (*oversamplingParam);
    prevOS = curOS;
    for (int i = 0; i < 5; ++i)
        overSample[i]->initProcessing ((size_t) samplesPerBlock);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    for (auto* gain : { &inGain, &outGain })
    {
        gain->prepare (spec);
        gain->setRampDurationSeconds (0.1);
    }

    dryWetMixer.prepare (spec);

    initializeProcessors (curOS);
}

void ProcessorChain::processAudio (AudioBuffer<float> buffer)
{
    SpinLock::ScopedTryLockType tryProcessingLock (processingLock);
    if (! tryProcessingLock.isLocked())
        return;

    // oversampling
    int curOS = static_cast<int> (*oversamplingParam);
    if (prevOS != curOS)
    {
        initializeProcessors (curOS);
        prevOS = curOS;
    }

    // set input, output, and dry/wet
    inGain.setGainDecibels (inGainParam->load());
    outGain.setGainDecibels (outGainParam->load());
    dryWetMixer.setDryWet (dryWetParam->load());

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);
    inGain.process (context);
    dryWetMixer.copyDryBuffer (buffer);

    auto osBlock = overSample[curOS]->processSamplesUp (block);

    // process mono or stereo buffer?
    const auto osNumSamples = (int) osBlock.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    const auto useMono = monoModeParam->load() == 0.0f;

    if (useMono)
    {
        monoBuffer.setSize (1, osNumSamples, false, false, true);
        monoBuffer.clear();
        monoBuffer.copyFrom (0, 0, osBlock.getChannelPointer (0), osNumSamples);

        for (int ch = 1; ch < numChannels; ++ch)
            monoBuffer.addFrom (0, 0, osBlock.getChannelPointer ((size_t) ch), osNumSamples);

        monoBuffer.applyGain (1.0f / (float) numChannels);
    }
    else
    {
        stereoBuffer.setSize (2, osNumSamples, false, false, true);
        stereoBuffer.clear();

        for (int ch = 0; ch < numChannels; ++ch)
            stereoBuffer.copyFrom (ch, 0, osBlock.getChannelPointer ((size_t) ch), osNumSamples);
    }

    auto& processBuffer = useMono ? monoBuffer : stereoBuffer;

    for (auto* processor : procs)
    {
        if (processor->isBypassed())
        {
            processor->processAudioBypassed (processBuffer);
            continue;
        }

        processor->processAudio (processBuffer);
    }

    // go back to original block of data
    if (useMono)
    {
        auto processedData = processBuffer.getReadPointer (0);
        for (int ch = 0; ch < numChannels; ++ch)
            FloatVectorOperations::copy (osBlock.getChannelPointer ((size_t) ch),
                                         processedData,
                                         osNumSamples);
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
            FloatVectorOperations::copy (osBlock.getChannelPointer ((size_t) ch),
                                         processBuffer.getReadPointer (ch),
                                         osNumSamples);
    }

    overSample[curOS]->processSamplesDown (block);

    auto latencySamples = overSample[curOS]->getLatencyInSamples();
    dryWetMixer.processBlock (buffer, latencySamples);
    outGain.process (context);
}

void ProcessorChain::addProcessor (BaseProcessor::Ptr newProc)
{
    um->beginNewTransaction();
    um->perform (new AddOrRemoveProcessor (*this, std::move (newProc)));
}

void ProcessorChain::removeProcessor (BaseProcessor* procToRemove)
{
    um->beginNewTransaction();
    um->perform (new AddOrRemoveProcessor (*this, procToRemove));
}

void ProcessorChain::moveProcessor (const BaseProcessor* procToMove, const BaseProcessor* procInSlot)
{
    if (procToMove == procInSlot)
        return;

    auto indexToMove = procs.indexOf (procToMove);
    auto slotIndex = procInSlot == nullptr ? procs.size() - 1 : procs.indexOf (procInSlot);

    um->beginNewTransaction();
    um->perform (new MoveProcessor (*this, indexToMove, slotIndex));
}

std::unique_ptr<XmlElement> ProcessorChain::saveProcChain()
{
    auto xml = std::make_unique<XmlElement> ("proc_chain");
    for (auto* proc : procs)
    {
        auto procXml = std::make_unique<XmlElement> (proc->getName().replaceCharacter (' ', '_'));
        procXml->addChildElement (proc->toXML().release());
        xml->addChildElement (procXml.release());
    }

    return std::move (xml);
}

void ProcessorChain::loadProcChain (XmlElement* xml)
{
    um->beginNewTransaction();

    while (! procs.isEmpty())
        um->perform (new AddOrRemoveProcessor (*this, procs.getLast()));

    for (auto* procXml : xml->getChildIterator())
    {
        auto newProc = procStore.createProcByName (procXml->getTagName().replaceCharacter ('_', ' '));
        if (newProc == nullptr)
        {
            jassertfalse;
            continue;
        }

        if (procXml->getNumChildElements() > 0)
            newProc->fromXML (procXml->getChildElement (0));

        um->perform (new AddOrRemoveProcessor (*this, std::move (newProc)));
    }
}
