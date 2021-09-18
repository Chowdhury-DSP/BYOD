#include "ProcessorChain.h"
#include "ParameterHelpers.h"
#include "ProcessorChainActions.h"

namespace
{
const String oversamplingTag = "oversampling";
const String inGainTag = "in_gain";
const String outGainTag = "out_gain";
const String dryWetTag = "dry_wet";

[[maybe_unused]] void printBufferLevels (const AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto level = buffer.getRMSLevel (ch, 0, numSamples);
        std::cout << "Level for channel: " << ch << ": " << level << std::endl;
    }
}

String getPortTag (int portIdx)
{
    return "port_" + String (portIdx);
}

String getConnectionTag (int connectionIdx)
{
    return "connection_" + String (connectionIdx);
}

String getProcessorTagName (const BaseProcessor* proc)
{
    return proc->getName().replaceCharacter (' ', '_');
}

String getProcessorName (const String& tag)
{
    return tag.replaceCharacter ('_', ' ');
}
} // namespace

ProcessorChain::ProcessorChain (ProcessorStore& store, AudioProcessorValueTreeState& vts) : procStore (store),
                                                                                            um (vts.undoManager),
                                                                                            inputProcessor (um),
                                                                                            outputProcessor (um)
{
    using namespace std::placeholders;
    procStore.addProcessorCallback = std::bind (&ProcessorChain::addProcessor, this, _1);

    oversamplingParam = vts.getRawParameterValue (oversamplingTag);
    inGainParam = vts.getRawParameterValue (inGainTag);
    outGainParam = vts.getRawParameterValue (outGainTag);
    dryWetParam = vts.getRawParameterValue (dryWetTag);

    for (int i = 0; i < 5; ++i)
        overSample[i] = std::make_unique<dsp::Oversampling<float>> (2, i, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
}

void ProcessorChain::createParameters (Parameters& params)
{
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

    inputProcessor.prepare (osSampleRate, osSamplesPerBlock);
    outputProcessor.prepare (osSampleRate, osSamplesPerBlock);

    for (auto* processor : procs)
        processor->prepare (osSampleRate, osSamplesPerBlock);
}

void ProcessorChain::prepare (double sampleRate, int samplesPerBlock)
{
    mySampleRate = sampleRate;
    mySamplesPerBlock = samplesPerBlock;

    inputBuffer.setSize (2, samplesPerBlock * 16); // allocate extra space for upsampled buffers

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

void ProcessorChain::runProcessor (BaseProcessor* proc, AudioBuffer<float>& buffer, bool& outProcessed)
{
    int nextNumProcs = 0;
    const int numOutputs = proc->getNumOutputs();
    for (int i = 0; i < numOutputs; ++i)
    {
        const int numOutProcs = proc->getNumOutputProcessors (i);
        for (int j = 0; j < numOutProcs; ++j)
            nextNumProcs += 1;
    }

    if (proc == &outputProcessor)
    {
        proc->processAudio (buffer);
        outProcessed = true;
        return;
    }

    if (nextNumProcs == 0)
        return;

    proc->processAudio (buffer);
    auto* outBuffer = &proc->getOutputBuffer();
    if (outBuffer == nullptr)
        outBuffer = &buffer;

    for (int i = numOutputs - 1; i >= 0; --i)
    {
        // @TODO: this approach won't really work when we have processors with multiple inputs...
        const int numOutProcs = proc->getNumOutputProcessors (i);
        for (int j = numOutProcs - 1; j >= 0; --j)
        {
            auto* nextProc = proc->getOutputProcessor (i, j);
            if (nextNumProcs == 1)
            {
                runProcessor (nextProc, *outBuffer, outProcessed);
            }
            else
            {
                auto& nextBuffer = nextProc->getInputBuffer();
                nextBuffer.makeCopyOf (*outBuffer, true);
                runProcessor (nextProc, nextBuffer, outProcessed);
            }

            nextNumProcs -= 1;
        }
    }
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

    inputBuffer.setSize (2, osNumSamples, false, false, true);
    inputBuffer.clear();

    for (int ch = 0; ch < numChannels; ++ch)
        inputBuffer.copyFrom (ch, 0, osBlock.getChannelPointer ((size_t) ch), osNumSamples);

    bool outProcessed = false;
    runProcessor (&inputProcessor, inputBuffer, outProcessed);

    if (outProcessed)
    {
        auto& outBuffer = outputProcessor.getOutputBuffer();
        for (int ch = 0; ch < numChannels; ++ch)
            FloatVectorOperations::copy (osBlock.getChannelPointer ((size_t) ch),
                                         outBuffer.getReadPointer (ch),
                                         osNumSamples);
    }
    else
    {
        osBlock.clear();
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

    auto removeConnections = [=] (BaseProcessor* proc)
    {
        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            int numConnections = proc->getNumOutputProcessors (portIdx);
            for (int cIdx = 0; cIdx < numConnections; ++cIdx)
            {
                if (proc->getOutputProcessor (portIdx, cIdx) == procToRemove)
                {
                    ConnectionInfo info { proc, portIdx, procToRemove, 0 }; // @TODO: fix for multi-input
                    um->perform (new AddOrRemoveConnection (*this, std::move (info), true));
                }
            }
        }
    };

    for (auto* proc : procs)
    {
        if (proc == procToRemove)
            continue;

        removeConnections (proc);
    }

    removeConnections (&inputProcessor);

    um->perform (new AddOrRemoveProcessor (*this, procToRemove));
}

void ProcessorChain::addConnection (ConnectionInfo&& info)
{
    um->beginNewTransaction();
    um->perform (new AddOrRemoveConnection (*this, std::move (info)));
}

void ProcessorChain::removeConnection (ConnectionInfo&& info)
{
    um->beginNewTransaction();
    um->perform (new AddOrRemoveConnection (*this, std::move (info), true));
}

std::unique_ptr<XmlElement> ProcessorChain::saveProcChain()
{
    auto xml = std::make_unique<XmlElement> ("proc_chain");

    auto saveProcessor = [&] (BaseProcessor* proc)
    {
        auto procXml = std::make_unique<XmlElement> (getProcessorTagName (proc));
        procXml->addChildElement (proc->toXML().release());

        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            auto numOutputs = proc->getNumOutputProcessors (portIdx);
            if (numOutputs == 0)
                continue;

            auto portElement = std::make_unique<XmlElement> (getPortTag (portIdx));
            for (int cIdx = 0; cIdx < numOutputs; ++cIdx)
            {
                if (const auto* connectedProc = proc->getOutputProcessor (portIdx, cIdx))
                {
                    auto processorIdx = procs.indexOf (connectedProc);

                    if (processorIdx == -1)
                    {
                        // there can only be one!
                        jassert (connectedProc == &outputProcessor);
                    }

                    portElement->setAttribute (getConnectionTag (cIdx), processorIdx);
                }
            }

            procXml->addChildElement (portElement.release());
        }

        xml->addChildElement (procXml.release());
    };

    for (auto* proc : procs)
        saveProcessor (proc);

    saveProcessor (&inputProcessor);

    return std::move (xml);
}

void ProcessorChain::loadProcChain (XmlElement* xml)
{
    um->beginNewTransaction();

    while (! procs.isEmpty())
        um->perform (new AddOrRemoveProcessor (*this, procs.getLast()));

    auto numInputConnections = inputProcessor.getNumOutputProcessors (0);
    while (numInputConnections > 0)
    {
        auto* endProc = inputProcessor.getOutputProcessor (0, numInputConnections - 1);
        ConnectionInfo info { &inputProcessor, 0, endProc, 0 }; // @TODO: make better for multi-input
        um->perform (new AddOrRemoveConnection (*this, std::move (info), true));
        numInputConnections = inputProcessor.getNumOutputProcessors (0);
    }

    using PortMap = std::vector<int>;
    using ProcConnectionMap = std::unordered_map<int, PortMap>;
    auto loadProcessorState = [=] (XmlElement* procXml, BaseProcessor* newProc, auto& connectionMaps)
    {
        if (procXml->getNumChildElements() > 0)
            newProc->fromXML (procXml->getChildElement (0));

        ProcConnectionMap connectionMap;
        for (int portIdx = 0; portIdx < newProc->getNumOutputs(); ++portIdx)
        {
            if (auto* portElement = procXml->getChildByName (getPortTag (portIdx)))
            {
                auto numConnections = portElement->getNumAttributes();
                std::vector<int> portConnections (numConnections);
                for (int cIdx = 0; cIdx < numConnections; ++cIdx)
                {
                    auto attributeName = portElement->getAttributeName (cIdx);
                    auto processorIdx = portElement->getIntAttribute (attributeName);
                    portConnections[cIdx] = processorIdx;
                }

                connectionMap.insert ({ portIdx, std::move (portConnections) });
            }
        }

        int procIdx = newProc == &inputProcessor ? -1 : procs.size();
        connectionMaps.insert ({ procIdx, std::move (connectionMap) });
    };

    std::unordered_map<int, ProcConnectionMap> connectionMaps;
    for (auto* procXml : xml->getChildIterator())
    {
        auto procName = getProcessorName (procXml->getTagName());
        if (procName == inputProcessor.getName())
        {
            loadProcessorState (procXml, &inputProcessor, connectionMaps);
            continue;
        }

        auto newProc = procStore.createProcByName (procName);
        if (newProc == nullptr)
        {
            jassertfalse;
            continue;
        }

        loadProcessorState (procXml, newProc.get(), connectionMaps);
        um->perform (new AddOrRemoveProcessor (*this, std::move (newProc)));
    }

    // wait until all the processors are created before connecting them
    for (auto [procIdx, connectionMap] : connectionMaps)
    {
        auto* proc = procIdx >= 0 ? procs[(int) procIdx] : &inputProcessor;
        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            if (connectionMap.find (portIdx) == connectionMap.end())
                continue; // no connections!

            const auto& connections = connectionMap.at (portIdx);
            for (auto cIdx : connections)
            {
                auto* procToConnect = cIdx >= 0 ? procs[cIdx] : &outputProcessor;
                ConnectionInfo info { proc, portIdx, procToConnect, 0 }; // @TODO: we need to save endIdx somewhere...
                um->perform (new AddOrRemoveConnection (*this, std::move (info)));
            }
        }
    }

    listeners.call (&Listener::refreshConnections);
}
