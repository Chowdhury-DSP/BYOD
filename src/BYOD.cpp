#include "BYOD.h"
#include "SystemInfo.h"
#include "gui/BoardViewport.h"
#include "gui/presets/PresetsItem.h"
#include "gui/utils/CPUMeter.h"
#include "gui/utils/LookAndFeels.h"
#include "gui/utils/SettingsButton.h"
#include "gui/utils/TextSliderItem.h"
#include "processors/chain/ProcessorChainStateHelper.h"
#include "state/presets/PresetManager.h"

namespace
{
const String settingsFilePath = "ChowdhuryDSP/BYOD/.plugin_settings.json";
const String logFileSubDir = "ChowdhuryDSP/BYOD/Logs";
const String logFileNameRoot = "BYOD_Log_";
} // namespace

BYOD::BYOD() : chowdsp::PluginBase<BYOD> (&undoManager),
               logger (logFileSubDir, logFileNameRoot),
               procStore (&undoManager)
{
    Logger::writeToLog (SystemInfo::getDiagnosticsString (*this));

    pluginSettings->initialise (settingsFilePath);

    procs = std::make_unique<ProcessorChain> (procStore, vts, presetManager, [&] (int l)
                                              { updateSampleLatency (l); });
    paramForwarder = std::make_unique<ParamForwardManager> (vts, *procs);

    presetManager = std::make_unique<PresetManager> (procs.get(), vts);

#if JUCE_IOS
    LookAndFeel::setDefaultLookAndFeel (lnfAllocator->getLookAndFeel<chowdsp::ChowLNF>());
#endif
}

void BYOD::addParameters (Parameters& params)
{
    ProcessorChain::createParameters (params);
}

void BYOD::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);

    procs->prepare (sampleRate, samplesPerBlock);
    loadMeasurer.reset (sampleRate, samplesPerBlock);

    bypassDelay.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    bypassScratchBuffer.setSize (2, samplesPerBlock);
}

void BYOD::releaseResources()
{
}

void BYOD::processAudioBlock (AudioBuffer<float>& buffer)
{
    AudioProcessLoadMeasurer::ScopedTimer loadTimer { loadMeasurer, buffer.getNumSamples() };

    // push samples into bypass delay
    bypassScratchBuffer.makeCopyOf (buffer);
    processBypassDelay (bypassScratchBuffer);

    // real processing here!
    procs->processAudio (buffer);
}

void BYOD::processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer&)
{
    AudioProcessLoadMeasurer::ScopedTimer loadTimer { loadMeasurer, buffer.getNumSamples() };

    processBypassDelay (buffer);
}

void BYOD::processBypassDelay (AudioBuffer<float>& buffer)
{
    auto&& block = dsp::AudioBlock<float> { buffer };
    bypassDelay.process (dsp::ProcessContextReplacing<float> { block });
}

void BYOD::updateSampleLatency (int latencySamples)
{
    setLatencySamples (latencySamples);
    bypassDelay.setDelay ((float) latencySamples);
}

AudioProcessorEditor* BYOD::createEditor()
{
    if (openGLHelper == nullptr)
        openGLHelper = std::make_unique<chowdsp::OpenGLHelper>();

    struct BYODInfoProvider : public chowdsp::StandardInfoProvider
    {
        static juce::String getWrapperTypeString (const BYOD& proc) { return proc.getWrapperTypeString(); }
    };

    auto builder = chowdsp::createGUIBuilder (magicState);
    builder->registerFactory ("Board", &BoardItem::factory);
    builder->registerFactory ("PresetsItem", &PresetsItem<BYOD>::factory);
    builder->registerFactory ("TextSlider", &TextSliderItem::factory);
    builder->registerFactory ("SettingsButton", &SettingsButtonItem::factory);
    builder->registerFactory ("CPUMeter", &CPUMeterItem<BYOD>::factory);
    builder->registerFactory ("OSMenu", &chowdsp::OversamplingMenuItem<BYOD>::factory);
    builder->registerFactory ("BYODInfoComp", &chowdsp::InfoItem<BYODInfoProvider, BYOD>::factory);
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

    openGLHelper->setComponent (editor);

    return editor;
}

String BYOD::getWrapperTypeString() const
{
#if HAS_CLAP_JUCE_EXTENSIONS
    // Since we are using 'external clap' this is the one JUCE API we can't override
    if (wrapperType == wrapperType_Undefined && is_clap)
        return "Clap";
#endif

    return AudioProcessor::getWrapperTypeDescription (wrapperType);
}

void BYOD::getStateInformation (MemoryBlock& destData)
{
    auto xml = std::make_unique<XmlElement> ("state");

    auto state = vts.copyState();
    xml->addChildElement (state.createXml().release());
    xml->addChildElement (procs->getStateHelper().saveProcChain().release());
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
    const auto presetWasDirty = presetManager->getIsDirty();

    vts.replaceState (ValueTree::fromXml (*vtsXml));
    procs->getStateHelper().loadProcChain (procChainXml);

    presetManager->setIsDirty (presetWasDirty);
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BYOD();
}
