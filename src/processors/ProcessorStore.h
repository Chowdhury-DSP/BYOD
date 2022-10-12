#pragma once

#include "BaseProcessor.h"

#if BYOD_ENABLE_ADD_ON_MODULES
class AddOnProcessorStore;
#endif

class ProcessorStore;
template <typename FilterType>
void createProcListFiltered (const ProcessorStore& store, PopupMenu& menu, int& menuID, FilterType&& filter, BaseProcessor* procToReplace = nullptr, ConnectionInfo* connectionInfo = nullptr);

class ProcessorStore
{
public:
    using StoreMap = std::map<String, std::function<BaseProcessor::Ptr (UndoManager*)>>;

    explicit ProcessorStore (UndoManager* um = nullptr);
    ~ProcessorStore();

    BaseProcessor::Ptr createProcByName (const String& name);
    void duplicateProcessor (BaseProcessor& procToDuplicate);

    void createProcList (PopupMenu& menu, int& menuID) const;
    void createProcReplaceList (PopupMenu& menu, int& menuID, BaseProcessor* procToReplace) const;
    void createProcFromCableClickList (PopupMenu& menu, int& menuID, ConnectionInfo* connectionInfo) const;

    static StoreMap& getStoreMap() { return store; }

    bool isModuleAvailable (const String& procName) const noexcept;

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

#if BYOD_ENABLE_ADD_ON_MODULES
    std::unique_ptr<AddOnProcessorStore> addOnProcessorStore;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorStore)
};
