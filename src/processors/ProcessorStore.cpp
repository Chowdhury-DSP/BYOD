#include "ProcessorStore.h"

#include "drive/BassFace.h"
#include "drive/BlondeDrive.h"
#include "drive/GuitarMLAmp.h"
#include "drive/MetalFace.h"
#include "drive/RONN.h"
#include "drive/RangeBooster.h"
#include "drive/Warp.h"
#include "drive/big_muff/BigMuffDrive.h"
#include "drive/centaur/Centaur.h"
#include "drive/diode_circuits/DiodeClipper.h"
#include "drive/diode_circuits/DiodeRectifier.h"
#include "drive/flapjack/Flapjack.h"
#include "drive/hysteresis/Hysteresis.h"
#include "drive/junior_b/JuniorB.h"
#include "drive/king_of_tone/KingOfToneDrive.h"
#include "drive/mouse_drive/MouseDrive.h"
#include "drive/muff_clipper/MuffClipper.h"
#include "drive/mxr_distortion/MXRDistortion.h"
#include "drive/tube_amp/TubeAmp.h"
#include "drive/tube_screamer/TubeScreamer.h"
#include "drive/waveshaper/Waveshaper.h"
#include "drive/zen_drive/ZenDrive.h"

#include "tone/BassCleaner.h"
#include "tone/BigMuffTone.h"
#include "tone/BlondeTone.h"
#include "tone/GraphicEQ.h"
#include "tone/HighCut.h"
#include "tone/LofiIrs.h"
#include "tone/StateVariableFilter.h"
#include "tone/TrebleBooster.h"
#include "tone/amp_irs/AmpIRs.h"
#include "tone/bassman/BassmanTone.h"
#include "tone/baxandall/BaxandallEQ.h"
#include "tone/ladder_filter/LadderFilterProcessor.h"
#include "tone/tube_screamer_tone/TubeScreamerTone.h"

#include "modulation/Chorus.h"
#include "modulation/Flanger.h"
#include "modulation/Panner.h"
#include "modulation/Rotary.h"
#include "modulation/Tremolo.h"
#include "modulation/phaser/Phaser4.h"
#include "modulation/phaser/Phaser8.h"
#include "modulation/scanner_vibrato/ScannerVibrato.h"
#include "modulation/uni_vibe/UniVibe.h"

#include "other/Compressor.h"
#include "other/Delay.h"
#include "other/EnvelopeFilter.h"
#include "other/Gate.h"
#include "other/Octaver.h"
#include "other/ShimmerReverb.h"
#include "other/SmoothReverb.h"
#include "other/cry_baby/CryBaby.h"
#include "other/spring_reverb/SpringReverbProcessor.h"

#include "utility/CleanGain.h"
#include "utility/DCBias.h"
#include "utility/DCBlocker.h"
#include "utility/FreqBandSplitter.h"
#include "utility/Mixer.h"
#include "utility/Oscilloscope.h"
#include "utility/StereoMerger.h"
#include "utility/StereoSplitter.h"
#include "utility/Tuner.h"

#if BYOD_ENABLE_ADD_ON_MODULES
#include <AddOnProcessorStore.h>
#include <AddOnProcessors.h>
#endif

template <typename ProcType>
static std::unique_ptr<BaseProcessor> processorFactory (UndoManager* um)
{
    return std::make_unique<ProcType> (um);
}

ProcessorStore::StoreMap ProcessorStore::store = {
    { "Bass Face", &processorFactory<BassFace> },
    { "Blonde Drive", &processorFactory<BlondeDrive> },
    { "Centaur", &processorFactory<Centaur> },
    { "Diode Clipper", &processorFactory<DiodeClipper> },
    { "Diode Rectifier", &processorFactory<DiodeRectifier> },
    { "Dirty Tube", &processorFactory<TubeAmp> },
    { "Distortion Plus", &processorFactory<MXRDistortion> },
    { "Flapjack", &processorFactory<Flapjack> },
    { "GuitarML", &processorFactory<GuitarMLAmp> },
    { "Hysteresis", &processorFactory<Hysteresis> },
    { "Junior B", &processorFactory<JuniorB> },
    { "Tone King", &processorFactory<KingOfToneDrive> },
    { "Metal Face", &processorFactory<MetalFace> },
    { "Mouse Drive", &processorFactory<MouseDrive> },
    { "Muff Clipper", &processorFactory<MuffClipper> },
    { "Muff Drive", &processorFactory<BigMuffDrive> },
    { "Range Booster", &processorFactory<RangeBooster> },
    { "RONN", &processorFactory<RONN> },
    { "Tube Screamer", &processorFactory<TubeScreamer> },
    { "Waveshaper", &processorFactory<Waveshaper> },
    { "Warp", &processorFactory<Warp> },
    { "Yen Drive", &processorFactory<ZenDrive> },

    { "Amp IRs", &processorFactory<AmpIRs> },
    { "Bass Cleaner", &processorFactory<BassCleaner> },
    { "Bassman Tone", &processorFactory<BassmanTone> },
    { "Baxandall EQ", &processorFactory<BaxandallEQ> },
    { "Blonde Tone", &processorFactory<BlondeTone> },
    { "Graphic EQ", &processorFactory<GraphicEQ> },
    { "High Cut", &processorFactory<HighCut> },
    { "LoFi IRs", &processorFactory<LofiIrs> },
    { "Muff Tone", &processorFactory<BigMuffTone> },
    { "SVF", &processorFactory<StateVariableFilter> },
    { "Treble Booster", &processorFactory<TrebleBooster> },
    { "TS-Tone", &processorFactory<TubeScreamerTone> },
    { "Ladder Filter", &processorFactory<LadderFilterProcessor> },

    { "Chorus", &processorFactory<Chorus> },
    { "Flanger", &processorFactory<Flanger> },
    { "Panner", &processorFactory<Panner> },
    { "Phaser4", &processorFactory<Phaser4> },
    { "Phaser8", &processorFactory<Phaser8> },
    { "Rotary", &processorFactory<Rotary> },
    { "Scanner Vibrato", &processorFactory<ScannerVibrato> },
    { "Solo-Vibe", &processorFactory<UniVibe> },
    { "Tremolo", &processorFactory<Tremolo> },

    { "Clean Gain", &processorFactory<CleanGain> },
    { "DC Bias", &processorFactory<DCBias> },
    { "DC Blocker", &processorFactory<DCBlocker> },
    { "Frequency Splitter", &processorFactory<FreqBandSplitter> },
    { "Mixer", &processorFactory<Mixer> },
    { "Oscilloscope", &processorFactory<Oscilloscope> },
    { "Stereo Merger", &processorFactory<StereoMerger> },
    { "Stereo Splitter", &processorFactory<StereoSplitter> },
    { "Tuner", &processorFactory<Tuner> },

    { "Compressor", &processorFactory<Compressor> },
    { "Crying Child", &processorFactory<CryBaby> },
    { "Delay", &processorFactory<DelayModule> },
    { "Envelope Filter", &processorFactory<EnvelopeFilter> },
    { "Gate", &processorFactory<Gate> },
    { "Octaver", &processorFactory<Octaver> },
    { "Shimmer Reverb", &processorFactory<ShimmerReverb> },
    { "Smooth Reverb", &processorFactory<SmoothReverb> },
    { "Spring Reverb", &processorFactory<SpringReverbProcessor> },

#if BYOD_ENABLE_ADD_ON_MODULES
    BYOD_STORE_MAP_ADD_ON_MODULES
#endif
};

ProcessorStore::ProcessorStore (UndoManager* um) : undoManager (um)
{
    // load processor info asynchronously
    std::vector<std::future<std::pair<String, ProcInfo>>> futureProcInfos;
    futureProcInfos.reserve (store.size());

    for (auto& [name, procFactory] : store)
    {
        futureProcInfos.push_back (std::async (std::launch::async,
                                               [this, name = name, procFactory = procFactory]
                                               {
                                                   auto proc = procFactory (undoManager);
                                                   jassert (name == proc->getName());

                                                   return std::make_pair (name, ProcInfo { proc->getProcessorType(), proc->getNumInputs(), proc->getNumOutputs() });
                                               }));
    }

    for (auto& f : futureProcInfos)
    {
        const auto& [name, info] = f.get();
        procTypeStore[name] = info;
    }

#if BYOD_ENABLE_ADD_ON_MODULES
    addOnProcessorStore = std::make_unique<AddOnProcessorStore>();
    addOnProcessorStore->validateModules (store);
#endif
}

ProcessorStore::~ProcessorStore() = default;

BaseProcessor::Ptr ProcessorStore::createProcByName (const String& name)
{
    if (store.find (name) == store.end())
        return {};

    return store[name](undoManager);
}

void ProcessorStore::duplicateProcessor (BaseProcessor& procToDuplicate)
{
    auto newProc = createProcByName (procToDuplicate.getName());
    if (newProc != nullptr)
        newProc->fromXML (procToDuplicate.toXML().get(), chowdsp::Version { std::string_view { JucePlugin_VersionString } }, false);

    addProcessorCallback (std::move (newProc));
}

bool ProcessorStore::isModuleAvailable (const String& procName) const noexcept
{
#if BYOD_ENABLE_ADD_ON_MODULES
    if (addOnProcessorStore->getModuleStatus (procName) == AddOnProcessorStore::ModuleStatus::AddOnModuleLocked)
        return false;
#endif

    juce::ignoreUnused (procName);
    return true;
}

template <typename FilterType>
void createProcListFiltered (const ProcessorStore& store, PopupMenu& menu, int& menuID, FilterType&& filter, BaseProcessor* procToReplace, ConnectionInfo* connectionInfo)
{
    if (procToReplace != nullptr
        && procToReplace->getNumInputs() == 1
        && procToReplace->getNumOutputs() == 1)
    {
        PopupMenu::Item item;
        item.itemID = ++menuID;
        item.text = "Nothing";
        item.action = [&store, procToReplace]
        {
            store.replaceProcessorCallback (nullptr, procToReplace);
        };

        menu.addItem (item);
    }

    for (auto type : { Drive, Tone, Modulation, Utility, Other })
    {
        PopupMenu subMenu;
        for (const auto& [procName, procCreator] : store.store)
        {
            const auto& procInfo = store.procTypeStore.at (procName);

            if (procInfo.type != type)
                continue;

            if (! filter (procName, procInfo))
                continue;

#if BYOD_ENABLE_ADD_ON_MODULES
            const auto ModuleStatus = store.addOnProcessorStore->getModuleStatus (procName);
            if (ModuleStatus == AddOnProcessorStore::ModuleStatus::AddOnModuleLocked)
                continue;
#endif

            PopupMenu::Item item;
            item.itemID = ++menuID;
            item.text = procName;
            if (procToReplace != nullptr)
                item.isEnabled = procName != procToReplace->getName();
#if BYOD_ENABLE_ADD_ON_MODULES
            item.colour = ModuleStatus == AddOnProcessorStore::ModuleStatus::AddOnModuleUnlocked ? Colours::gold : Colours::white;
#else
            item.colour = Colours::white;
#endif

            item.action = [&store, &procCreator = procCreator, procToReplace, connectionInfo]
            {
                if (connectionInfo != nullptr)
                    store.replaceConnectionWithProcessorCallback (procCreator (store.undoManager), *connectionInfo);
                else if (procToReplace != nullptr)
                    store.replaceProcessorCallback (procCreator (store.undoManager), procToReplace);
                else
                    store.addProcessorCallback (procCreator (store.undoManager));
            };

            subMenu.addItem (item);
        }

        if (subMenu.containsAnyActiveItems())
        {
            auto typeName = std::string (magic_enum::enum_name (type));
            menu.addSubMenu (String (typeName), subMenu);
        }
    }
}

void createProcListUnfiltered (const ProcessorStore& store, PopupMenu& menu, int& menuID)
{
    createProcListFiltered (
        store, menu, menuID, [] (auto...)
        { return true; });
}

void ProcessorStore::createProcList (PopupMenu& menu, int& menuID) const
{
    createProcListUnfiltered (*this, menu, menuID);
}

void ProcessorStore::createProcReplaceList (PopupMenu& menu, int& menuID, BaseProcessor* procToReplace) const
{
    createProcListFiltered (
        *this,
        menu,
        menuID,
        [procToReplace] ([[maybe_unused]] const String& name, const ProcInfo& procInfo)
        {
            return procInfo.numInputs == procToReplace->getNumInputs()
                   && procInfo.numOutputs == procToReplace->getNumOutputs();
        },
        procToReplace);
}

void ProcessorStore::createProcFromCableClickList (PopupMenu& menu, int& menuID, ConnectionInfo* connectionInfo) const
{
    createProcListFiltered (
        *this,
        menu,
        menuID,
        [] (auto, const ProcInfo& procInfo)
        {
            return procInfo.numInputs == 1 && procInfo.numOutputs == 1;
        },
        nullptr,
        connectionInfo);
}
