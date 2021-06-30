#include "BYOD.h"
#include "gui/BoardViewport.h"
#include "gui/utils/BottomBarLNF.h"

BYOD::BYOD() : chowdsp::PluginBase<BYOD> (&undoManager),
               procStore (&undoManager),
               procs (procStore, vts)
{
}

void BYOD::addParameters (Parameters& params)
{
    ProcessorChain::createParameters (params);
}

void BYOD::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    procs.prepare (sampleRate, samplesPerBlock);
}

void BYOD::releaseResources()
{
}

void BYOD::processAudioBlock (AudioBuffer<float>& buffer)
{
    procs.processAudio (buffer);
}

AudioProcessorEditor* BYOD::createEditor()
{
    auto builder = chowdsp::createGUIBuilder (magicState);
    builder->registerFactory ("Board", &BoardItem::factory);
    builder->registerLookAndFeel ("BottomBar", std::make_unique<BottomBarLNF>());

    // GUI trigger functions
    magicState.addTrigger ("undo", [=]
                           { undoManager.undo(); });
    magicState.addTrigger ("redo", [=]
                           { undoManager.redo(); });

    auto editor = new foleys::MagicPluginEditor (magicState, BinaryData::gui_xml, BinaryData::gui_xmlSize, std::move (builder));

    // we need to set resize limits for StandalonePluginHolder
    editor->setResizeLimits (10, 10, 2000, 2000);

    return editor;
}

void BYOD::getStateInformation (MemoryBlock& destData)
{
    MessageManagerLock mml;

    auto xml = std::make_unique<XmlElement> ("state");

    auto state = vts.copyState();
    xml->addChildElement (state.createXml().release());
    xml->addChildElement (procs.saveProcChain().release());

    copyXmlToBinary (*xml, destData);
}

void BYOD::setStateInformation (const void* data, int sizeInBytes)
{
    MessageManagerLock mml;
    auto xmlState = getXmlFromBinary (data, sizeInBytes);

    if (xmlState == nullptr) // invalid XML
        return;

    auto vtsXml = xmlState->getChildByName (vts.state.getType());
    if (vtsXml == nullptr) // invalid ValueTreeState
        return;

    auto procChainXml = xmlState->getChildByName ("proc_chain");
    if (procChainXml == nullptr) // invalid procChain XML
        return;

    vts.replaceState (ValueTree::fromXml (*vtsXml));
    procs.loadProcChain (procChainXml);
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BYOD();
}
