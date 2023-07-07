#include "processors/ProcessorStore.h"

class ProcessorStoreInfoTest : public UnitTest
{
public:
    ProcessorStoreInfoTest()
        : UnitTest ("Processor Store Info Test")
    {
    }

    void runTest() override
    {
        for (const auto& [name, storeEntry] : ProcessorStore::getStoreMap())
        {
            const auto proc = storeEntry.factory (nullptr);
            beginTest ("Checking processor info for: " + name);
            expectEquals (proc->getName(), name, "Processor name is incorrect!");
            expectEquals (proc->getProcessorType(), storeEntry.info.type, "Processor type is incorrect!");
            expectEquals (proc->getNumInputs(), storeEntry.info.numInputs, "Processor # inputs is incorrect!");
            expectEquals (proc->getNumOutputs(), storeEntry.info.numOutputs, "Processor # outputs is incorrect!");
        }
    }
};

static ProcessorStoreInfoTest processorStoreInfoTest;
