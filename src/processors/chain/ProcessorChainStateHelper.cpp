#include "ProcessorChainStateHelper.h"
#include "ProcessorChainActions.h"
#include "state/ParamForwardManager.h"
#include "state/presets/PresetManager.h"

namespace ChainStateHelperFuncs
{
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

ProcessorChainStateHelper::ProcessorChainStateHelper (ProcessorChain& thisChain, chowdsp::DeferredAction& deferredAction)
    : chain (thisChain),
      um (chain.um),
      mainThreadStateLoader (deferredAction)
{
}

void ProcessorChainStateHelper::loadProcChain (const XmlElement* xml,
                                               const chowdsp::Version& stateVersion,
                                               bool loadingPreset,
                                               Component* associatedComponent,
                                               WaitableEvent* waiter)
{
    if (xml == nullptr)
    {
        jassertfalse; // something has gone wrong!
        return;
    }

    mainThreadStateLoader.call (
        [this,
         stateVersion,
         loadingPreset,
         xmlState = *xml,
         safeComp = Component::SafePointer { associatedComponent },
         waiter]
        {
            loadProcChainInternal (&xmlState, stateVersion, loadingPreset, safeComp.getComponent());
            if (waiter != nullptr)
                waiter->signal();
        });
}

std::unique_ptr<XmlElement> ProcessorChainStateHelper::saveProcChain()
{
    auto xml = std::make_unique<XmlElement> (procChainStateTag);

    auto saveProcessor = [&] (BaseProcessor* proc)
    {
        auto procXml = std::make_unique<XmlElement> (ChainStateHelperFuncs::getProcessorTagName (proc));
        procXml->addChildElement (proc->toXML().release());

        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            auto numOutputs = proc->getNumOutputConnections (portIdx);
            if (numOutputs == 0)
                continue;

            auto portElement = std::make_unique<XmlElement> (ChainStateHelperFuncs::getPortTag (portIdx));
            for (int cIdx = 0; cIdx < numOutputs; ++cIdx)
            {
                auto& connection = proc->getOutputConnection (portIdx, cIdx);
                auto processorIdx = chain.procs.indexOf (connection.endProc);

                if (processorIdx == -1)
                {
                    // there can only be one!
                    jassert (connection.endProc == &chain.outputProcessor);
                }

                portElement->setAttribute (ChainStateHelperFuncs::getConnectionTag (cIdx), processorIdx);
                portElement->setAttribute (ChainStateHelperFuncs::getConnectionEndTag (cIdx), connection.endPort);
            }

            procXml->addChildElement (portElement.release());
        }

        xml->addChildElement (procXml.release());
    };

    for (auto* proc : chain.procs)
        saveProcessor (proc);

    saveProcessor (&chain.inputProcessor);
    saveProcessor (&chain.outputProcessor);

    return std::move (xml);
}

void ProcessorChainStateHelper::loadProcChainInternal (const XmlElement* xml,
                                                       const chowdsp::Version& stateVersion,
                                                       bool loadingPreset,
                                                       Component* associatedComp)
{
    ParamForwardManager::ScopedForceDeferHostNotifications scopedDeferNotifs { *chain.paramForwardManager };

    if (! loadingPreset)
        um->beginNewTransaction();

    for (auto* proc : chain.procs)
        ProcessorChainHelpers::removeOutputConnectionsFromProcessor (chain, proc, chain.um);
    ProcessorChainHelpers::removeOutputConnectionsFromProcessor (chain, &chain.inputProcessor, chain.um);

    while (! chain.procs.isEmpty())
        um->perform (new AddOrRemoveProcessor (chain, chain.procs.getLast()));

    using PortMap = std::vector<std::pair<int, int>>;
    using ProcConnectionMap = std::unordered_map<int, PortMap>;
    auto loadProcessorState = [this, &stateVersion] (XmlElement* procXml, BaseProcessor* newProc, auto& connectionMaps, bool shouldLoadState = true)
    {
        if (procXml->getNumChildElements() > 0)
        {
            if (shouldLoadState)
                newProc->fromXML (procXml->getChildElement (0), stateVersion);
            else // don't load state, only load position
                newProc->loadPositionInfoFromXML (procXml->getChildElement (0));
        }

        ProcConnectionMap connectionMap;
        for (int portIdx = 0; portIdx < newProc->getNumOutputs(); ++portIdx)
        {
            if (auto* portElement = procXml->getChildByName (ChainStateHelperFuncs::getPortTag (portIdx)))
            {
                auto numConnections = portElement->getNumAttributes() / 2;
                PortMap portConnections (numConnections);
                for (int cIdx = 0; cIdx < numConnections; ++cIdx)
                {
                    auto processorIdx = portElement->getIntAttribute (ChainStateHelperFuncs::getConnectionTag (cIdx));
                    auto endPort = portElement->getIntAttribute (ChainStateHelperFuncs::getConnectionEndTag (cIdx));
                    portConnections[cIdx] = std::make_pair (processorIdx, endPort);
                }

                connectionMap.insert ({ portIdx, std::move (portConnections) });
            }
        }

        if (newProc == &chain.outputProcessor)
            return;

        int procIdx = newProc == &chain.inputProcessor ? -1 : chain.procs.size();
        connectionMaps.insert ({ procIdx, std::move (connectionMap) });
    };

    std::unordered_map<int, ProcConnectionMap> connectionMaps;
    StringArray unavailableProcessors;
    for (auto* procXml : xml->getChildIterator())
    {
        if (procXml == nullptr)
        {
            jassertfalse;
            continue;
        }

        const auto procName = ChainStateHelperFuncs::getProcessorName (procXml->getTagName());
        if (procName == chain.inputProcessor.getName())
        {
            loadProcessorState (procXml, &chain.inputProcessor, connectionMaps, ! loadingPreset);
            continue;
        }

        if (procName == chain.outputProcessor.getName())
        {
            loadProcessorState (procXml, &chain.outputProcessor, connectionMaps, ! loadingPreset);
            continue;
        }

        if (! chain.procStore.isModuleAvailable (procName))
        {
            Logger::writeToLog ("Skipping loading processor: " + procName + ", since it is currently locked!");
            unavailableProcessors.addIfNotAlreadyThere (procName);
            continue;
        }

        auto newProc = chain.procStore.createProcByName (procName);
        if (newProc == nullptr)
        {
            jassertfalse; // unable to create this processor
            continue;
        }

        loadProcessorState (procXml, newProc.get(), connectionMaps);
        um->perform (new AddOrRemoveProcessor (chain, std::move (newProc)));
    }

    if (loadingPreset && ! unavailableProcessors.isEmpty())
    {
        std::stringstream warningStream;
        warningStream << "Error loading preset! The following processors were unavailable:\n";
        for (const auto& name : unavailableProcessors)
            warningStream << name << '\n';
        warningStream.seekp (-1, std::ios_base::end); // remove the trailing newline
        PresetManager::showErrorMessage ("Error Loading Preset", warningStream.str(), associatedComp);
    }

    // wait until all the processors are created before connecting them
    for (auto [procIdx, connectionMap] : connectionMaps)
    {
        auto* proc = procIdx >= 0 ? chain.procs[(int) procIdx] : &chain.inputProcessor;
        if (proc == nullptr)
        {
            jassertfalse;
            continue;
        }

        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            if (connectionMap.find (portIdx) == connectionMap.end())
                continue; // no connections!

            const auto& connections = connectionMap.at (portIdx);
            for (auto [cIdx, endPort] : connections)
            {
                if (auto* procToConnect = cIdx >= 0 ? chain.procs[cIdx] : &chain.outputProcessor; procToConnect != nullptr && procToConnect != proc)
                {
                    ConnectionInfo info { proc, portIdx, procToConnect, endPort };
                    um->perform (new AddOrRemoveConnection (chain, std::move (info)));
                }
            }
        }
    }

    chain.refreshConnectionsBroadcaster();
}

bool ProcessorChainStateHelper::validateProcChainState (const XmlElement* xml, const ProcessorStore& procStore)
{
    if (xml == nullptr)
        return false;

    if (xml->getTagName() != procChainStateTag)
        return false;

    for (auto* procXml : xml->getChildIterator())
    {
        const auto procName = ChainStateHelperFuncs::getProcessorName (procXml->getTagName());
        if (procName == chowdsp::toString (InputProcessor::name) || procName == chowdsp::toString (OutputProcessor::name))
            continue;

        if (! procStore.isModuleAvailable (procName))
        {
            Logger::writeToLog ("State contains processor: " + procName + ", which is currently locked!");
            return false;
        }
    }

    return true;
}
