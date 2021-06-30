#pragma once

#include "BaseProcessor.h"

class ProcessorStore
{
    using StoreMap = std::map<String, std::function<BaseProcessor::Ptr (UndoManager*)>>;
public:
    ProcessorStore (UndoManager* um = nullptr);

    BaseProcessor::Ptr createProcByName (const String& name);
    void createProcList (PopupMenu& menu, int& menuID, ProcessorType type);

    StoreMap& getStoreMap() { return store; }

    std::function<void (BaseProcessor::Ptr)> addProcessorCallback = nullptr;

private:
    static StoreMap store;

    std::unordered_map<String, ProcessorType> procTypeStore;
    UndoManager* undoManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorStore)
};
