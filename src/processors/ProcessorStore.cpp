#include "ProcessorStore.h"

#include "drive/TanhDrive.h"

template <typename ProcType>
static std::unique_ptr<BaseProcessor> processorFactory()
{
    // @TODO: forward constructor args
    return std::make_unique<ProcType>();
}

ProcessorStore::StoreMap ProcessorStore::store = {
    { "TanhDrive", &processorFactory<TanhDrive> }
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
