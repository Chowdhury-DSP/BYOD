#include "ProcessorStore.h"

#include "drive/DiodeClipper.h"
#include "tone/HighCut.h"

#include "utility/CleanGain.h"
#include "utility/DCBias.h"
#include "utility/DCBlocker.h"

template <typename ProcType>
static std::unique_ptr<BaseProcessor> processorFactory()
{
    // @TODO: forward constructor args
    return std::make_unique<ProcType>();
}

ProcessorStore::StoreMap ProcessorStore::store = {
    { "Diode Clipper", &processorFactory<DiodeClipper> },
    { "High Cut", &processorFactory<HighCut> },
    { "Clean Gain", &processorFactory<CleanGain> },
    { "DC Bias", &processorFactory<DCBias> },
    { "DC Blocker", &processorFactory<DCBlocker> },
};

ProcessorStore::ProcessorStore()
{
    for (auto& [name, procFactory] : store)
    {
        auto proc = procFactory();
        jassert (name == proc->getName());
        procTypeStore[name] = proc->getProcessorType();
    }
}

BaseProcessor::Ptr ProcessorStore::createProcByName (const String& name)
{
    if (store.find (name) == store.end())
        return {};

    return store[name]();
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
        item.action = [=] { addProcessorCallback (procDesc.second()); };

        menu.addItem (item);
    }
}
