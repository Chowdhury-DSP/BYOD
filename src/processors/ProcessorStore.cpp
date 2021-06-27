#include "ProcessorStore.h"

#include "drive/diode_circuits/DiodeClipper.h"
#include "drive/diode_circuits/DiodeRectifier.h"
#include "drive/zen_drive/ZenDrive.h"

#include "tone/BassCleaner.h"
#include "tone/HighCut.h"
#include "tone/TrebleBooster.h"

#include "utility/CleanGain.h"
#include "utility/DCBias.h"
#include "utility/DCBlocker.h"

template <typename ProcType>
static std::unique_ptr<BaseProcessor> processorFactory (UndoManager* um)
{
    return std::make_unique<ProcType> (um);
}

ProcessorStore::StoreMap ProcessorStore::store = {
    { "Diode Clipper", &processorFactory<DiodeClipper> },
    { "Diode Rectifier", &processorFactory<DiodeRectifier> },
    { "Zen Drive", &processorFactory<ZenDrive> },
    { "Bass Cleaner", &processorFactory<BassCleaner> },
    { "High Cut", &processorFactory<HighCut> },
    { "Treble Booster", &processorFactory<TrebleBooster> },
    { "Clean Gain", &processorFactory<CleanGain> },
    { "DC Bias", &processorFactory<DCBias> },
    { "DC Blocker", &processorFactory<DCBlocker> },
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

    return store[name] (undoManager);
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
        item.action = [=] { addProcessorCallback (procDesc.second (undoManager)); };

        menu.addItem (item);
    }
}
