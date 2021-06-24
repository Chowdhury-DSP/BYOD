#include "ProcessorChain.h"

namespace
{
    const String monoModeTag = "mono_stereo";
    const String oversamplingTag = "oversampling";
}

ProcessorChain::ProcessorChain (ProcessorStore& store, AudioProcessorValueTreeState& vts) : procStore (store)
{
    using namespace std::placeholders;
    procStore.addProcessorCallback = std::bind (&ProcessorChain::addProcessor, this, _1);

    monoModeParam = vts.getRawParameterValue (monoModeTag);
    oversamplingParam = vts.getRawParameterValue (oversamplingTag);

    for (int i = 0; i < 5; ++i)
        overSample[i] = std::make_unique<dsp::Oversampling<float>> (2, i, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
}

void ProcessorChain::createParameters (Parameters& params)
{
    params.push_back (std::make_unique<AudioParameterChoice> (monoModeTag,
                                                              "Mono/Stereo",
                                                              StringArray { "Mono", "Stereo" },
                                                              1));

    params.push_back (std::make_unique<AudioParameterChoice> (oversamplingTag,
                                                              "Oversampling",
                                                              StringArray { "1x", "2x", "4x", "8x", "16x" },
                                                              1));
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

    dsp::AudioBlock<float> block (buffer);
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
                                         processedData, osNumSamples);
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
            FloatVectorOperations::copy (osBlock.getChannelPointer ((size_t) ch),
                                         processBuffer.getReadPointer (ch),
                                         osNumSamples);
    }

    overSample[curOS]->processSamplesDown (block);
}

void ProcessorChain::addProcessor (BaseProcessor::Ptr newProc)
{
    DBG (String ("Creating processor: ") + newProc->getName());

    newProc->prepare (mySampleRate, mySamplesPerBlock);

    SpinLock::ScopedLockType scopedProcessingLock (processingLock);
    auto* newProcPtr = procs.add (std::move (newProc));

    listeners.call (&Listener::processorAdded, newProcPtr);
}

void ProcessorChain::removeProcessor (const BaseProcessor* procToRemove)
{
    DBG (String ("Removing processor: ") + procToRemove->getName());

    listeners.call (&Listener::processorRemoved, procToRemove);

    SpinLock::ScopedLockType scopedProcessingLock (processingLock);
    procs.removeObject (procToRemove);
}

void ProcessorChain::moveProcessor (const BaseProcessor* procToMove, const BaseProcessor* procInSlot)
{
    if (procToMove == procInSlot)
        return;

    auto indexToMove = procs.indexOf (procToMove);
    auto slotIndex = procInSlot == nullptr ? procs.size() - 1 : procs.indexOf (procInSlot);
    procs.move (indexToMove, slotIndex);

    listeners.call (&Listener::processorMoved, indexToMove, slotIndex);
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
    while (! procs.isEmpty())
        removeProcessor (procs.getLast());

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

        addProcessor (std::move (newProc));
    }
}
