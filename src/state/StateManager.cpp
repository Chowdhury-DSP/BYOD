#include "StateManager.h"
#include "gui/GUIConstants.h"
#include "processors/chain/ProcessorChainStateHelper.h"

namespace
{
const Identifier statePluginVersionTag = "state_plugin_version";
}

StateManager::StateManager (AudioProcessorValueTreeState& vtState, ProcessorChain& procs, chowdsp::PresetManager& presetMgr)
    : vts (vtState),
      procChain (procs),
      presetManager (presetMgr),
      uiState (vts, GUIConstants::defaultWidth, GUIConstants::defaultHeight),
      pluginWrapperType (vts.processor.wrapperType)
{
}

void StateManager::setCurrentPluginVersionInXML (XmlElement* xml)
{
    xml->setAttribute (statePluginVersionTag, JucePlugin_VersionString);
}

chowdsp::Version StateManager::getPluginVersionFromXML (const XmlElement* xml)
{
    return chowdsp::Version { xml->getStringAttribute (statePluginVersionTag, "1.0.1") };
}

std::unique_ptr<XmlElement> StateManager::saveState()
{
    auto xml = std::make_unique<XmlElement> ("state");

    auto state = vts.copyState();
    xml->addChildElement (state.createXml().release());
    xml->addChildElement (procChain.getStateHelper().saveProcChain().release());
    xml->addChildElement (presetManager.saveXmlState().release());
    setCurrentPluginVersionInXML (xml.get());

    return xml;
}

void StateManager::loadState (XmlElement* xmlState)
{
    if (xmlState == nullptr) // invalid XML
        return;

    auto vtsXml = xmlState->getChildByName (vts.state.getType());
    if (vtsXml == nullptr) // invalid ValueTreeState
        return;

    auto procChainXml = xmlState->getChildByName ("proc_chain");
    if (procChainXml == nullptr) // invalid procChain XML
        return;

    const auto [presetWasDirty, pluginVersion] = [&]
    {
        std::optional<MessageManagerLock> mml {};
        if (pluginWrapperType != AudioProcessor::WrapperType::wrapperType_AAX)
            mml.emplace();
        presetManager.loadXmlState (xmlState->getChildByName (chowdsp::PresetManager::presetStateTag));
        const auto wasDirty = presetManager.getIsDirty();

        const auto savedPluginVersion = getPluginVersionFromXML (xmlState);
        vts.replaceState (ValueTree::fromXml (*vtsXml));

        return std::make_tuple (wasDirty, savedPluginVersion);
    }();

    std::unique_ptr<WaitableEvent> waiter;
    if (pluginWrapperType != AudioProcessor::WrapperType::wrapperType_AAX)
        waiter = std::make_unique<WaitableEvent>();
    procChain.getStateHelper().loadProcChain (procChainXml, pluginVersion, false, nullptr, waiter.get());
    if (waiter != nullptr)
        waiter->wait (1000);

    std::optional<MessageManagerLock> mml {};
    if (pluginWrapperType != AudioProcessor::WrapperType::wrapperType_AAX)
        mml.emplace();
    presetManager.setIsDirty (presetWasDirty);

    if (auto* um = vts.undoManager)
        um->clearUndoHistory();
}
