#include "StateManager.h"
#include "gui/GUIConstants.h"
#include "processors/chain/ProcessorChainStateHelper.h"

StateManager::StateManager (AudioProcessorValueTreeState& vtState, ProcessorChain& procs, chowdsp::PresetManager& presetMgr)
    : vts (vtState),
      procChain (procs),
      presetManager (presetMgr),
      uiState (vts, GUIConstants::defaultWidth, GUIConstants::defaultHeight)
{
}

std::unique_ptr<XmlElement> StateManager::saveState()
{
    auto xml = std::make_unique<XmlElement> ("state");

    auto state = vts.copyState();
    xml->addChildElement (state.createXml().release());
    xml->addChildElement (procChain.getStateHelper().saveProcChain().release());
    xml->addChildElement (presetManager.saveXmlState().release());

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

    presetManager.loadXmlState (xmlState->getChildByName (chowdsp::PresetManager::presetStateTag));
    const auto presetWasDirty = presetManager.getIsDirty();

    vts.replaceState (ValueTree::fromXml (*vtsXml));
    procChain.getStateHelper().loadProcChain (procChainXml);

    presetManager.setIsDirty (presetWasDirty);

    if (auto* um = vts.undoManager)
        um->clearUndoHistory();
}
