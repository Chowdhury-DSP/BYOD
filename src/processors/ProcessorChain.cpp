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

String getConnectionEndTag (int connectionIdx)
{
    return "connection_end_" + String (connectionIdx);
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

    auto prepProcessor = [=] (BaseProcessor& proc) {
        proc.prepare (osSampleRate, osSamplesPerBlock);
        proc.prepareInputBuffers (osSamplesPerBlock);
    };

    prepProcessor (inputProcessor);
    prepProcessor (outputProcessor);

    for (auto* processor : procs)
        prepProcessor (*processor);
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
        const int numOutProcs = proc->getNumOutputConnections (i);
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

    auto processBuffer = [&] (BaseProcessor* nextProc, AudioBuffer<float>& nextBuffer) {
        int nextNumInputs = nextProc->getNumInputs();
        if (nextNumProcs == 1 && nextNumInputs == 1)
        {
            runProcessor (nextProc, nextBuffer, outProcessed);
        }
        else if (nextNumProcs > 1 && nextNumInputs == 1)
        {
            auto& copyNextBuffer = nextProc->getInputBuffer();
            copyNextBuffer.makeCopyOf (nextBuffer, true);
            runProcessor (nextProc, copyNextBuffer, outProcessed);
        }
        else
        {
            int inputIdx = nextProc->getNextInputIdx();
            auto& copyNextBuffer = nextProc->getInputBuffer (inputIdx);
            copyNextBuffer.makeCopyOf (nextBuffer, true);

            if (nextProc->getNumInputsReady() < nextProc->getNumInputConnections())
                return; // not all the inputs are ready yet...

            runProcessor (nextProc, copyNextBuffer, outProcessed);
            nextProc->clearInputIdx();
        }
    };

    for (int i = numOutputs - 1; i >= 0; --i)
    {
        auto* outBuffer = proc->getOutputBuffer (i);
        if (outBuffer == nullptr)
            outBuffer = &buffer;

        const int numOutProcs = proc->getNumOutputConnections (i);
        for (int j = numOutProcs - 1; j >= 0; --j)
        {
            auto* nextProc = proc->getOutputProcessor (i, j);
            processBuffer (nextProc, *outBuffer);

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
        auto& outBuffer = *outputProcessor.getOutputBuffer();
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

    auto removeConnections = [=] (BaseProcessor* proc) {
        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            int numConnections = proc->getNumOutputConnections (portIdx);
            for (int cIdx = numConnections - 1; cIdx >= 0; --cIdx)
            {
                auto connection = proc->getOutputConnection (portIdx, cIdx);
                if (connection.endProc == procToRemove)
                    um->perform (new AddOrRemoveConnection (*this, std::move (connection), true));
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

    auto saveProcessor = [&] (BaseProcessor* proc) {
        auto procXml = std::make_unique<XmlElement> (getProcessorTagName (proc));
        procXml->addChildElement (proc->toXML().release());

        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            auto numOutputs = proc->getNumOutputConnections (portIdx);
            if (numOutputs == 0)
                continue;

            auto portElement = std::make_unique<XmlElement> (getPortTag (portIdx));
            for (int cIdx = 0; cIdx < numOutputs; ++cIdx)
            {
                auto& connection = proc->getOutputConnection (portIdx, cIdx);
                auto processorIdx = procs.indexOf (connection.endProc);

                if (processorIdx == -1)
                {
                    // there can only be one!
                    jassert (connection.endProc == &outputProcessor);
                }

                portElement->setAttribute (getConnectionTag (cIdx), processorIdx);
                portElement->setAttribute (getConnectionEndTag (cIdx), connection.endPort);
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

void ProcessorChain::loadProcChain (const XmlElement* xml)
{
    um->beginNewTransaction();

    while (! procs.isEmpty())
        um->perform (new AddOrRemoveProcessor (*this, procs.getLast()));

    auto numInputConnections = inputProcessor.getNumOutputConnections (0);
    while (numInputConnections > 0)
    {
        auto connection = inputProcessor.getOutputConnection (0, numInputConnections - 1);
        um->perform (new AddOrRemoveConnection (*this, std::move (connection), true));
        numInputConnections = inputProcessor.getNumOutputConnections (0);
    }

    using PortMap = std::vector<std::pair<int, int>>;
    using ProcConnectionMap = std::unordered_map<int, PortMap>;
    auto loadProcessorState = [=] (XmlElement* procXml, BaseProcessor* newProc, auto& connectionMaps) {
        if (procXml->getNumChildElements() > 0)
            newProc->fromXML (procXml->getChildElement (0));

        ProcConnectionMap connectionMap;
        for (int portIdx = 0; portIdx < newProc->getNumOutputs(); ++portIdx)
        {
            if (auto* portElement = procXml->getChildByName (getPortTag (portIdx)))
            {
                auto numConnections = portElement->getNumAttributes() / 2;
                PortMap portConnections (numConnections);
                for (int cIdx = 0; cIdx < numConnections; ++cIdx)
                {
                    auto processorIdx = portElement->getIntAttribute (getConnectionTag (cIdx));
                    auto endPort = portElement->getIntAttribute (getConnectionEndTag (cIdx));
                    portConnections[cIdx] = std::make_pair (processorIdx, endPort);
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
            for (auto [cIdx, endPort] : connections)
            {
                auto* procToConnect = cIdx >= 0 ? procs[cIdx] : &outputProcessor;
                ConnectionInfo info { proc, portIdx, procToConnect, endPort };
                um->perform (new AddOrRemoveConnection (*this, std::move (info)));
            }
        }
    }

    listeners.call (&Listener::refreshConnections);
}
