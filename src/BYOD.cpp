#include "BYOD.h"
#include "gui/BoardViewport.h"
#include "gui/utils/CPUMeter.h"
#include "gui/utils/LookAndFeels.h"
#include "gui/utils/TextSliderItem.h"
#include "processors/chain/ProcessorChainStateHelper.h"
#include "state/PresetManager.h"

namespace
{
const String logFileSubDir = "ChowdhuryDSP/BYOD/Logs";
const String logFileNameRoot = "BYOD_Log_";
} // namespace

BYOD::BYOD() : chowdsp::PluginBase<BYOD> (&undoManager),
               logger (logFileSubDir, logFileNameRoot),
               procStore (&undoManager),
               procs (procStore, vts, presetManager),
               paramForwarder (vts, procs)
{
    presetManager = std::make_unique<PresetManager> (&procs, vts);
}

void BYOD::addParameters (Parameters& params)
{
    ProcessorChain::createParameters (params);
}

void BYOD::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    procs.prepare (sampleRate, samplesPerBlock);
    loadMeasurer.reset (sampleRate, samplesPerBlock);
}

void BYOD::releaseResources()
{
}

void BYOD::processAudioBlock (AudioBuffer<float>& buffer)
{
    AudioProcessLoadMeasurer::ScopedTimer loadTimer { loadMeasurer, buffer.getNumSamples() };

    procs.processAudio (buffer);
}

AudioProcessorEditor* BYOD::createEditor()
{
    auto builder = chowdsp::createGUIBuilder (magicState);
    builder->registerFactory ("Board", &BoardItem::factory);
    builder->registerFactory ("PresetsItem", &chowdsp::PresetsItem<BYOD>::factory);
    builder->registerFactory ("TextSlider", &TextSliderItem::factory);
    builder->registerFactory ("CPUMeter", &CPUMeterItem<BYOD>::factory);
    builder->registerLookAndFeel ("ByodLNF", std::make_unique<ByodLNF>());
    builder->registerLookAndFeel ("CPUMeterLNF", std::make_unique<CPUMeterLNF>());

    // GUI trigger functions
    magicState.addTrigger ("undo", [=]
                           { undoManager.undo(); });
    magicState.addTrigger ("redo", [=]
                           { undoManager.redo(); });

#if JUCE_IOS
    auto editor = new foleys::MagicPluginEditor (magicState, BinaryData::gui_ios_xml, BinaryData::gui_ios_xmlSize, std::move (builder));
#else
    auto editor = new foleys::MagicPluginEditor (magicState, BinaryData::gui_xml, BinaryData::gui_xmlSize, std::move (builder));
#endif

    // we need to set resize limits for StandalonePluginHolder
    editor->setResizeLimits (10, 10, 2000, 2000);

    return editor;
}

void BYOD::getStateInformation (MemoryBlock& destData)
{
    auto xml = std::make_unique<XmlElement> ("state");

    auto state = vts.copyState();
    xml->addChildElement (state.createXml().release());
    xml->addChildElement (procs.getStateHelper().saveProcChain().release());
    xml->addChildElement (presetManager->saveXmlState().release());

    copyXmlToBinary (*xml, destData);
}

void BYOD::setStateInformation (const void* data, int sizeInBytes)
{
    auto xmlState = getXmlFromBinary (data, sizeInBytes);

    if (xmlState == nullptr) // invalid XML
        return;

    auto vtsXml = xmlState->getChildByName (vts.state.getType());
    if (vtsXml == nullptr) // invalid ValueTreeState
        return;

    auto procChainXml = xmlState->getChildByName ("proc_chain");
    if (procChainXml == nullptr) // invalid procChain XML
        return;

    presetManager->loadXmlState (xmlState->getChildByName (chowdsp::PresetManager::presetStateTag));
    vts.replaceState (ValueTree::fromXml (*vtsXml));
    procs.getStateHelper().loadProcChain (procChainXml);
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BYOD();
}
