#pragma once

#include "BaseProcessor.h"

class ProcessorStore;
template <typename FilterType>
void createProcListFiltered (const ProcessorStore& store, PopupMenu& menu, int& menuID, FilterType&& filter, BaseProcessor* procToReplace = nullptr, ConnectionInfo* connectionInfo = nullptr);

class ProcessorStore
{
public:
    using StoreMap = std::map<String, std::function<BaseProcessor::Ptr (UndoManager*)>>;

    explicit ProcessorStore (UndoManager* um = nullptr);

    BaseProcessor::Ptr createProcByName (const String& name);
    void duplicateProcessor (BaseProcessor& procToDuplicate);

    void createProcList (PopupMenu& menu, int& menuID) const;
    void createProcReplaceList (PopupMenu& menu, int& menuID, BaseProcessor* procToReplace) const;
    void createProcFromCableClickList (PopupMenu& menu, int& menuID, ConnectionInfo* connectionInfo) const;

    static StoreMap& getStoreMap() { return store; }

private:
    friend class ProcessorChainActionHelper;

    static StoreMap store;

    std::function<void (BaseProcessor::Ptr)> addProcessorCallback = nullptr;
    std::function<void (BaseProcessor::Ptr, BaseProcessor*)> replaceProcessorCallback = nullptr;
    std::function<void (BaseProcessor::Ptr, ConnectionInfo& connectionInfo)> replaceConnectionWithProcessorCallback = nullptr;

    struct ProcInfo
    {
        ProcessorType type;
        int numInputs;
        int numOutputs;
    };

    std::unordered_map<String, ProcInfo> procTypeStore;
    UndoManager* undoManager;

    template <typename FilterType>
    friend void createProcListFiltered (const ProcessorStore&, PopupMenu&, int&, FilterType&&, BaseProcessor*, ConnectionInfo*);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorStore)
};
