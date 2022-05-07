#include "BYOD.h"
#include "SystemInfo.h"
#include "gui/BYODPluginEditor.h"
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
    stateManager = std::make_unique<StateManager> (vts, *procs, *presetManager);

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
    bypassScratchBuffer.makeCopyOf (buffer, true);
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

    auto* editor = new BYODPluginEditor (*this);
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
    copyXmlToBinary (*stateManager->saveState(), destData);
}

void BYOD::setStateInformation (const void* data, int sizeInBytes)
{
    stateManager->loadState (getXmlFromBinary (data, sizeInBytes).get());
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BYOD();
}
