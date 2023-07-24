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
#include "drive/fuzz_machine/FuzzMachine.h"
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
#include "modulation/MIDIModulator.h"
#include "modulation/Panner.h"
#include "modulation/ParamModulator.h"
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
#include "other/krusher/Krusher.h"

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
    { "Bass Face", { .factory = &processorFactory<BassFace>, .info = { ProcessorType::Drive, 1, 1 } } },
    { "Blonde Drive", { &processorFactory<BlondeDrive>, { ProcessorType::Drive, 1, 1 } } },
    { "Centaur", { &processorFactory<Centaur>, { ProcessorType::Drive, 1, 1 } } },
    { "Diode Clipper", { &processorFactory<DiodeClipper>, { ProcessorType::Drive, 1, 1 } } },
    { "Diode Rectifier", { &processorFactory<DiodeRectifier>, { ProcessorType::Drive, 1, 1 } } },
    { "Dirty Tube", { &processorFactory<TubeAmp>, { ProcessorType::Drive, 1, 1 } } },
    { "Distortion Plus", { &processorFactory<MXRDistortion>, { ProcessorType::Drive, 1, 1 } } },
    { "Flapjack", { &processorFactory<Flapjack>, { ProcessorType::Drive, 1, 1 } } },
    { "Fuzz Machine", { &processorFactory<FuzzMachine>, { ProcessorType::Drive, 1, 1 } } },
    { "GuitarML", { &processorFactory<GuitarMLAmp>, { ProcessorType::Drive, 1, 1 } } },
    { "Hysteresis", { &processorFactory<Hysteresis>, { ProcessorType::Drive, 1, 1 } } },
    { "Junior B", { &processorFactory<JuniorB>, { ProcessorType::Drive, 1, 1 } } },
    { "Tone King", { &processorFactory<KingOfToneDrive>, { ProcessorType::Drive, 1, 1 } } },
    { "Metal Face", { &processorFactory<MetalFace>, { ProcessorType::Drive, 1, 1 } } },
    { "Mouse Drive", { &processorFactory<MouseDrive>, { ProcessorType::Drive, 1, 1 } } },
    { "Muff Clipper", { &processorFactory<MuffClipper>, { ProcessorType::Drive, 1, 1 } } },
    { "Muff Drive", { &processorFactory<BigMuffDrive>, { ProcessorType::Drive, 1, 1 } } },
    { "Range Booster", { &processorFactory<RangeBooster>, { ProcessorType::Drive, 1, 1 } } },
    { "RONN", { &processorFactory<RONN>, { ProcessorType::Drive, 1, 1 } } },
    { "Tube Screamer", { &processorFactory<TubeScreamer>, { ProcessorType::Drive, 1, 1 } } },
    { "Waveshaper", { &processorFactory<Waveshaper>, { ProcessorType::Drive, 1, 1 } } },
    { "Warp", { &processorFactory<Warp>, { ProcessorType::Drive, 1, 1 } } },
    { "Yen Drive", { &processorFactory<ZenDrive>, { ProcessorType::Drive, 1, 1 } } },

    { "Amp IRs", { &processorFactory<AmpIRs>, { ProcessorType::Tone, 1, 1 } } },
    { "Bass Cleaner", { &processorFactory<BassCleaner>, { ProcessorType::Tone, 1, 1 } } },
    { "Bassman Tone", { &processorFactory<BassmanTone>, { ProcessorType::Tone, 1, 1 } } },
    { "Baxandall EQ", { &processorFactory<BaxandallEQ>, { ProcessorType::Tone, 1, 1 } } },
    { "Blonde Tone", { &processorFactory<BlondeTone>, { ProcessorType::Tone, 1, 1 } } },
    { "Graphic EQ", { &processorFactory<GraphicEQ>, { ProcessorType::Tone, 1, 1 } } },
    { "High Cut", { &processorFactory<HighCut>, { ProcessorType::Tone, 1, 1 } } },
    { "LoFi IRs", { &processorFactory<LofiIrs>, { ProcessorType::Tone, 1, 1 } } },
    { "Muff Tone", { &processorFactory<BigMuffTone>, { ProcessorType::Tone, 1, 1 } } },
    { "SVF", { &processorFactory<StateVariableFilter>, { ProcessorType::Tone, 1, 1 } } },
    { "Treble Booster", { &processorFactory<TrebleBooster>, { ProcessorType::Tone, 1, 1 } } },
    { "TS-Tone", { &processorFactory<TubeScreamerTone>, { ProcessorType::Tone, 1, 1 } } },
    { "Ladder Filter", { &processorFactory<LadderFilterProcessor>, { ProcessorType::Tone, 1, 1 } } },

    { "Chorus", { &processorFactory<Chorus>, { ProcessorType::Modulation, Chorus::numInputs, Chorus::numOutputs } } },
    { "Flanger", { &processorFactory<Flanger>, { ProcessorType::Modulation, Flanger::numInputs, Flanger::numOutputs } } },
    { "MIDI Modulator", { &processorFactory<MidiModulator>, { ProcessorType::Modulation, 0, 1 } } },
    { "Panner", { &processorFactory<Panner>, { ProcessorType::Modulation, Panner::numInputs, Panner::numOutputs } } },
    { "Param Modulator", { &processorFactory<ParamModulator>, { ProcessorType::Modulation, 0, 1 } } },
    { "Phaser4", { &processorFactory<Phaser4>, { ProcessorType::Modulation, Phaser4::numInputs, Phaser4::numOutputs } } },
    { "Phaser8", { &processorFactory<Phaser8>, { ProcessorType::Modulation, Phaser8::numInputs, Phaser8::numOutputs } } },
    { "Rotary", { &processorFactory<Rotary>, { ProcessorType::Modulation, Rotary::numInputs, Rotary::numOutputs } } },
    { "Scanner Vibrato", { &processorFactory<ScannerVibrato>, { ProcessorType::Modulation, ScannerVibrato::numInputs, ScannerVibrato::numOutputs } } },
    { "Solo-Vibe", { &processorFactory<UniVibe>, { ProcessorType::Modulation, UniVibe::numInputs, UniVibe::numOutputs } } },
    { "Tremolo", { &processorFactory<Tremolo>, { ProcessorType::Modulation, Tremolo::numInputs, Tremolo::numOutputs } } },

    { "Clean Gain", { &processorFactory<CleanGain>, { ProcessorType::Utility, 1, 1 } } },
    { "DC Bias", { &processorFactory<DCBias>, { ProcessorType::Utility, 1, 1 } } },
    { "DC Blocker", { &processorFactory<DCBlocker>, { ProcessorType::Utility, 1, 1 } } },
    { "Frequency Splitter", { &processorFactory<FreqBandSplitter>, { ProcessorType::Utility, 1, FreqBandSplitter::numOuts } } },
    { "Mixer", { &processorFactory<Mixer>, { ProcessorType::Utility, Mixer::numIns, 1 } } },
    { "Oscilloscope", { &processorFactory<Oscilloscope>, { ProcessorType::Utility, 1, 0 } } },
    { "Stereo Merger", { &processorFactory<StereoMerger>, { ProcessorType::Utility, 2, 1 } } },
    { "Stereo Splitter", { &processorFactory<StereoSplitter>, { ProcessorType::Utility, 1, 2 } } },
    { "Tuner", { &processorFactory<Tuner>, { ProcessorType::Utility, 1, 0 } } },

    { "Compressor", { &processorFactory<Compressor>, { ProcessorType::Other, Compressor::numInputs, Compressor::numOutputs } } },
    { "Crying Child", { &processorFactory<CryBaby>, { ProcessorType::Other, CryBaby::numInputs, CryBaby::numOutputs } } },
    { "Delay", { &processorFactory<DelayModule>, { ProcessorType::Other, 1, 1 } } },
    { "Envelope Filter", { &processorFactory<EnvelopeFilter>, { ProcessorType::Other, EnvelopeFilter::numInputs, EnvelopeFilter::numOutputs } } },
    { "Gate", { &processorFactory<Gate>, { ProcessorType::Other, Gate::numInputs, Gate::numOutputs } } },
    { "Octaver", { &processorFactory<Octaver>, { ProcessorType::Other, 1, 1 } } },
    { "Shimmer Reverb", { &processorFactory<ShimmerReverb>, { ProcessorType::Other, 1, 1 } } },
    { "Smooth Reverb", { &processorFactory<SmoothReverb>, { ProcessorType::Other, 1, 1 } } },
    { "Spring Reverb", { &processorFactory<SpringReverbProcessor>, { ProcessorType::Other, 1, 1 } } },
    { "Krusher", { &processorFactory<Krusher>, { ProcessorType::Other, 1, 1 } } },

#if BYOD_ENABLE_ADD_ON_MODULES
    BYOD_STORE_MAP_ADD_ON_MODULES
#endif
};

ProcessorStore::ProcessorStore (UndoManager* um) : undoManager (um)
{
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

    return store[name].factory ((undoManager));
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
        for (const auto& [procName, storeEntry] : store.store)
        {
            const auto& procCreator = storeEntry.factory;
            const auto& procInfo = storeEntry.info;

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

static void createProcListUnfiltered (const ProcessorStore& store, PopupMenu& menu, int& menuID)
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
        [procToReplace] ([[maybe_unused]] const String& name, const StoreEntryInfo& procInfo)
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
        [] (auto, const StoreEntryInfo& procInfo)
        {
            return procInfo.numInputs == 1 && procInfo.numOutputs == 1;
        },
        nullptr,
        connectionInfo);
}
