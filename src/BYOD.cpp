#include "BYOD.h"
#include "gui/BoardViewport.h"
#include "gui/utils/LookAndFeels.h"
#include "gui/utils/TextSliderItem.h"
#include "presets/PresetCompItem.h"

BYOD::BYOD() : chowdsp::PluginBase<BYOD> (&undoManager),
               procStore (&undoManager),
               procs (procStore, vts),
               presetManager (&procs, vts)
{
    presetManager.hostUpdateFunc = [=] {
        MessageManager::callAsync ([=] { updateHostDisplay (AudioProcessorListener::ChangeDetails().withProgramChanged (true)); });
    };
}

void BYOD::addParameters (Parameters& params)
{
    ProcessorChain::createParameters (params);
    PresetManager::addParameters (params);
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
    builder->registerFactory ("PresetsItem", &PresetCompItem::factory);
    builder->registerFactory ("TextSlider", &TextSliderItem::factory);
    builder->registerLookAndFeel ("ByodLNF", std::make_unique<ByodLNF>());

    // GUI trigger functions
    magicState.addTrigger ("undo", [=] { undoManager.undo(); });
    magicState.addTrigger ("redo", [=] { undoManager.redo(); });

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

int BYOD::getNumPrograms()
{
    return presetManager.getNumFactoryPresets();
}

int BYOD::getCurrentProgram()
{
    const auto curPresetIdx = presetManager.getSelectedPresetIdx();

    if (curPresetIdx > presetManager.getNumFactoryPresets())
        return 0;

    return curPresetIdx;
}

void BYOD::setCurrentProgram (int index)
{
    if (index > presetManager.getNumPresets() || index < 0) // out of range!
        return;

    if (index == presetManager.getSelectedPresetIdx()) // no update needed!
        return;

    MessageManager::callAsync ([=] { presetManager.setPreset (index); });
}

const String BYOD::getProgramName (int index)
{
    return presetManager.getPresetName (index);
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BYOD();
}
