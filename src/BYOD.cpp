#include "BYOD.h"
#include "gui/BYODPluginEditor.h"
#include "state/StateManager.h"
#include "state/presets/PresetManager.h"

namespace BYODPaths
{
const String settingsFilePath = "ChowdhuryDSP/BYOD/.plugin_settings.json";
const String logFileSubDir = "ChowdhuryDSP/BYOD/Logs";
const String logFileNameRoot = "BYOD_Log_";
} // namespace

BYOD::BYOD() : chowdsp::PluginBase<BYOD> (&undoManager),
               logger ({ .logFileSubDir = BYODPaths::logFileSubDir,
                         .logFileNameRoot = BYODPaths::logFileNameRoot,
                         .crashLogAnalysisCallback = [this] (const File& logFile)
                         { crashLogFile.emplace (logFile); } }),
               procStore (&undoManager)
{
#if PERFETTO
    MelatoninPerfetto::get().beginSession();
#endif

    Logger::writeToLog (chowdsp::PluginDiagnosticInfo::getDiagnosticsString (*this));

    pluginSettings->initialise (BYODPaths::settingsFilePath);
    procs = std::make_unique<ProcessorChain> (procStore, vts, presetManager, paramForwarder, [&] (int l)
                                              { updateSampleLatency (l); });
    paramForwarder = std::make_unique<ParamForwardManager> (vts, *procs);
    presetManager = std::make_unique<PresetManager> (procs.get(), vts);
    stateManager = std::make_unique<StateManager> (vts, *procs, *presetManager);

#if JUCE_IOS
    LookAndFeel::setDefaultLookAndFeel (lnfAllocator->getLookAndFeel<chowdsp::ChowLNF>());
#endif
}

#if PERFETTO
BYOD::~BYOD()
{
    MelatoninPerfetto::get().endSession();
}
#else
BYOD::~BYOD() = default;
#endif

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

void BYOD::memoryWarningReceived()
{
    const MessageManagerLock mml {};
    undoManager.clearUndoHistory();
}

void BYOD::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    TRACE_DSP();

    const juce::ScopedNoDenormals noDenormals {};
    AudioProcessLoadMeasurer::ScopedTimer loadTimer { loadMeasurer, buffer.getNumSamples() };

    //get playhead
    procs->getPlayheadHelper().process (getPlayHead(), buffer.getNumSamples());

    // push samples into bypass delay
    bypassScratchBuffer.makeCopyOf (buffer, true);
    processBypassDelay (bypassScratchBuffer);

    // real processing here!
    procs->processAudio (buffer, midi);

    chowdsp::BufferMath::sanitizeBuffer<AudioBuffer<float>, float> (buffer);
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
    stateManager->getUIState().attachToComponent (*editor);
    openGLHelper->setComponent (editor);

    if (crashLogFile.has_value())
    {
        ErrorMessageView::showCrashLogView (*crashLogFile, editor);
        crashLogFile.reset();
    }

    return editor;
}

void BYOD::getStateInformation (MemoryBlock& destData)
{
    copyXmlToBinary (*stateManager->saveState(), destData);
}

void BYOD::setStateInformation (const void* data, int sizeInBytes)
{
    stateManager->loadState (getXmlFromBinary (data, sizeInBytes).get());

    if (wrapperType == WrapperType::wrapperType_AudioUnitv3)
    {
        // In the AUv3 we need to alert the host about parameter tree changes
        // _after_ the host has finished loading the plugin state.
        Timer::callAfterDelay (500, [this]
                               { updateHostDisplay (ChangeDetails {}.withParameterInfoChanged (true)); });
    }
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BYOD();
}

#if HAS_CLAP_JUCE_EXTENSIONS
#include "state/presets/PresetDiscovery.h"

bool BYOD::presetLoadFromLocation (uint32_t location_kind, const char* location, const char* load_key) noexcept
{
    return preset_discovery::presetLoadFromLocation (*presetManager, location_kind, location, load_key);
}

static const clap_preset_discovery_factory byod_preset_discovery_factory {
    .count = preset_discovery::count,
    .get_descriptor = preset_discovery::get_descriptor,
    .create = preset_discovery::create,
};

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wmissing-prototypes")
const void* JUCE_CALLTYPE clapJuceExtensionCustomFactory (const char* factory_id)
{
    if (strcmp (factory_id, CLAP_PRESET_DISCOVERY_FACTORY_ID) == 0)
    {
        return &byod_preset_discovery_factory;
    }
    return nullptr;
}
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#endif
