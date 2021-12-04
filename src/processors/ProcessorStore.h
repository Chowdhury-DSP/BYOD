#pragma once

#include "BaseProcessor.h"

class ProcessorStore
{
    using StoreMap = std::map<String, std::function<BaseProcessor::Ptr (UndoManager*)>>;

public:
    explicit ProcessorStore (UndoManager* um = nullptr);

    BaseProcessor::Ptr createProcByName (const String& name);
    void createProcList (PopupMenu& menu, int& menuID, ProcessorType type);
    void createProcReplaceList (PopupMenu& menu, int& menuID, ProcessorType type, BaseProcessor* procToReplace);

    static StoreMap& getStoreMap() { return store; }

    std::function<void (BaseProcessor::Ptr)> addProcessorCallback = nullptr;
    std::function<void (BaseProcessor::Ptr, BaseProcessor*)> replaceProcessorCallback = nullptr;

private:
    static StoreMap store;

    struct ProcInfo
    {
        ProcessorType type;
        int numInputs;
        int numOutputs;
    };

    std::unordered_map<String, ProcInfo> procTypeStore;
    UndoManager* undoManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorStore)
};
