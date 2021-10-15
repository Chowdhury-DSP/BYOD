#include "ProcessorStore.h"

#include "drive/GuitarMLAmp.h"
#include "drive/RONN.h"
#include "drive/centaur/Centaur.h"
#include "drive/diode_circuits/DiodeClipper.h"
#include "drive/diode_circuits/DiodeRectifier.h"
#include "drive/hysteresis/Hysteresis.h"
#include "drive/mxr_distortion/MXRDistortion.h"
#include "drive/tube_amp/TubeAmp.h"
#include "drive/tube_screamer/TubeScreamer.h"
#include "drive/zen_drive/ZenDrive.h"

#include "other/Chorus.h"
#include "other/Delay.h"
#include "other/EnvelopeFilter.h"
#include "other/Tremolo.h"
#include "other/spring_reverb/SpringReverbProcessor.h"

#include "tone/AmpIRs.h"
#include "tone/BassCleaner.h"
#include "tone/HighCut.h"
#include "tone/LofiIrs.h"
#include "tone/TrebleBooster.h"
#include "tone/bassman/BassmanTone.h"

#include "utility/CleanGain.h"
#include "utility/DCBias.h"
#include "utility/DCBlocker.h"
#include "utility/Mixer.h"
#include "utility/StereoSplitter.h"

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
    { "GuitarML", &processorFactory<GuitarMLAmp> },
    { "Hysteresis", &processorFactory<Hysteresis> },
    { "MXR Distortion", &processorFactory<MXRDistortion> },
    { "RONN", &processorFactory<RONN> },
    { "Tube Screamer", &processorFactory<TubeScreamer> },
    { "Zen Drive", &processorFactory<ZenDrive> },

    { "Bass Cleaner", &processorFactory<BassCleaner> },
    { "Bassman Tone", &processorFactory<BassmanTone> },
    { "High Cut", &processorFactory<HighCut> },
    { "Amp IRs", &processorFactory<AmpIRs> },
    { "LoFi IRs", &processorFactory<LofiIrs> },
    { "Treble Booster", &processorFactory<TrebleBooster> },

    { "Clean Gain", &processorFactory<CleanGain> },
    { "DC Bias", &processorFactory<DCBias> },
    { "DC Blocker", &processorFactory<DCBlocker> },
    { "Mixer", &processorFactory<Mixer> },
    { "Stereo Splitter", &processorFactory<StereoSplitter> },

    { "Chorus", &processorFactory<Chorus> },
    { "Delay", &processorFactory<Delay> },
    { "Envelope Filter", &processorFactory<EnvelopeFilter> },
    { "Tremolo", &processorFactory<Tremolo> },
    { "Spring Reverb", &processorFactory<SpringReverbProcessor> },
};

ProcessorStore::ProcessorStore (UndoManager* um) : undoManager (um)
{
    for (auto& [name, procFactory] : store)
    {
        auto proc = procFactory (undoManager);
        jassert (name == proc->getName());
        procTypeStore[name] = proc->getProcessorType();
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
        if (procTypeStore[procDesc.first] != type)
            continue;

        PopupMenu::Item item;
        item.itemID = ++menuID;
        item.text = procDesc.first;
        item.action = [=]
        { addProcessorCallback (procDesc.second (undoManager)); };

        menu.addItem (item);
    }
}
