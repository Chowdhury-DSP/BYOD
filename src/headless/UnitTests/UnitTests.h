#include "../../BYOD.h"

static inline void doForAllProcessors (std::function<void(BaseProcessor*)> testFunc)
{
    ProcessorStore procStore;
    for (auto [name, factory] : procStore.getStoreMap())
    {
        auto proc = factory (nullptr);
        testFunc (proc.get());
    }
}

class UnitTests : public ConsoleApplication::Command
{
public:
    UnitTests();

    void runUnitTests (const ArgumentList& args);

private:
    int64 getRandomSeed (const ArgumentList& args);
    Array<UnitTest*> getTestsForArgs (const ArgumentList& args);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnitTests)
};
