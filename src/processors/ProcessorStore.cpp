#include "ProcessorStore.h"

#include "drive/GuitarMLAmp.h"
#include "drive/RONN.h"
#include "drive/RangeBooster.h"
#include "drive/Warp.h"
#include "drive/big_muff/BigMuffDrive.h"
#include "drive/centaur/Centaur.h"
#include "drive/diode_circuits/DiodeClipper.h"
#include "drive/diode_circuits/DiodeRectifier.h"
#include "drive/hysteresis/Hysteresis.h"
#include "drive/metal_face/MetalFace.h"
#include "drive/mxr_distortion/MXRDistortion.h"
#include "drive/tube_amp/TubeAmp.h"
#include "drive/tube_screamer/TubeScreamer.h"
#include "drive/waveshaper/Waveshaper.h"
#include "drive/zen_drive/ZenDrive.h"

#include "other/Chorus.h"
#include "other/Compressor.h"
#include "other/Delay.h"
#include "other/EnvelopeFilter.h"
#include "other/Gate.h"
#include "other/Tremolo.h"
#include "other/spring_reverb/SpringReverbProcessor.h"

#include "tone/AmpIRs.h"
#include "tone/BassCleaner.h"
#include "tone/BigMuffTone.h"
#include "tone/GraphicEQ.h"
#include "tone/HighCut.h"
#include "tone/LofiIrs.h"
#include "tone/StateVariableFilter.h"
#include "tone/TrebleBooster.h"
#include "tone/bassman/BassmanTone.h"
#include "tone/baxandall/BaxandallEQ.h"
#include "tone/tube_screamer_tone/TubeScreamerTone.h"

#include "utility/CleanGain.h"
#include "utility/DCBias.h"
#include "utility/DCBlocker.h"
#include "utility/FreqBandSplitter.h"
#include "utility/Mixer.h"
#include "utility/Oscilloscope.h"
#include "utility/StereoMerger.h"
#include "utility/StereoSplitter.h"
#include "utility/Tuner.h"

template <typename ProcType>
static std::unique_ptr<BaseProcessor> processorFactory (UndoManager* um)
{
    return std::make_unique<ProcType> (um);
}

ProcessorStore::StoreMap ProcessorStore::store = {
    { "Centaur", &processorFactory<Centaur> },
    { "Diode Clipper", &processorFactory<DiodeClipper> },
    { "Diode Rectifier", &processorFactory<DiodeRectifier> },
    { "Dirty Tube", &processorFactory<TubeAmp> },
    { "Distortion Plus", &processorFactory<MXRDistortion> },
    { "GuitarML", &processorFactory<GuitarMLAmp> },
    { "Hysteresis", &processorFactory<Hysteresis> },
    { "Metal Face", &processorFactory<MetalFace> },
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
    { "Graphic EQ", &processorFactory<GraphicEQ> },
    { "High Cut", &processorFactory<HighCut> },
    { "LoFi IRs", &processorFactory<LofiIrs> },
    { "Muff Tone", &processorFactory<BigMuffTone> },
    { "SVF", &processorFactory<StateVariableFilter> },
    { "Treble Booster", &processorFactory<TrebleBooster> },
    { "TS-Tone", &processorFactory<TubeScreamerTone> },

    { "Clean Gain", &processorFactory<CleanGain> },
    { "DC Bias", &processorFactory<DCBias> },
    { "DC Blocker", &processorFactory<DCBlocker> },
    { "Frequency Splitter", &processorFactory<FreqBandSplitter> },
    { "Mixer", &processorFactory<Mixer> },
    { "Oscilloscope", &processorFactory<Oscilloscope> },
    { "Stereo Merger", &processorFactory<StereoMerger> },
    { "Stereo Splitter", &processorFactory<StereoSplitter> },
    { "Tuner", &processorFactory<Tuner> },

    { "Chorus", &processorFactory<Chorus> },
    { "Compressor", &processorFactory<Compressor> },
    { "Delay", &processorFactory<Delay> },
    { "Envelope Filter", &processorFactory<EnvelopeFilter> },
    { "Gate", &processorFactory<Gate> },
    { "Tremolo", &processorFactory<Tremolo> },
    { "Spring Reverb", &processorFactory<SpringReverbProcessor> },
};

ProcessorStore::ProcessorStore (UndoManager* um) : undoManager (um)
{
    // load processor info asynchronously
    std::vector<std::future<std::pair<String, ProcInfo>>> futureProcInfos;
    futureProcInfos.reserve (store.size());

    for (auto& [name, procFactory] : store)
    {
        futureProcInfos.push_back (std::async (std::launch::async, [this, name = name, procFactory = procFactory]
                                               {
                                                   auto proc = procFactory (undoManager);
                                                   jassert (name == proc->getName());

                                                   return std::make_pair (name, ProcInfo { proc->getProcessorType(), proc->getNumInputs(), proc->getNumOutputs() }); }));
    }

    for (auto& f : futureProcInfos)
    {
        const auto& [name, info] = f.get();
        procTypeStore[name] = info;
    }
}

BaseProcessor::Ptr ProcessorStore::createProcByName (const String& name)
{
    if (store.find (name) == store.end())
        return {};

    return store[name](undoManager);
}

void ProcessorStore::createProcList (PopupMenu& menu, int& menuID, ProcessorType type)
{
    for (auto& procDesc : store)
    {
        const auto& procInfo = procTypeStore[procDesc.first];
        if (procInfo.type != type)
            continue;

        PopupMenu::Item item;
        item.itemID = ++menuID;
        item.text = procDesc.first;
        item.action = [=]
        { addProcessorCallback (procDesc.second (undoManager)); };

        menu.addItem (item);
    }
}

void ProcessorStore::createProcReplaceList (PopupMenu& menu, int& menuID, ProcessorType type, BaseProcessor* procToReplace)
{
    for (auto& procDesc : store)
    {
        const auto& procInfo = procTypeStore[procDesc.first];
        if (procInfo.type != type)
            continue;

        if (procInfo.numInputs != procToReplace->getNumInputs())
            continue;

        if (procInfo.numOutputs != procToReplace->getNumOutputs())
            continue;

        PopupMenu::Item item;
        item.itemID = ++menuID;
        item.text = procDesc.first;
        item.action = [=]
        { replaceProcessorCallback (procDesc.second (undoManager), procToReplace); };

        menu.addItem (item);
    }
}
