#include "../../BYOD.h"

static inline void runTestForAllProcessors (UnitTest* ut, const std::function<void (BaseProcessor*)>& testFunc, const StringArray& procsToSkip = {})
{
    for (auto [name, factory] : ProcessorStore::getStoreMap())
    {
        if (procsToSkip.contains (name))
        {
            std::cout << "Skipping " << ut->getName() << " for processor: " << name << std::endl;
            continue;
        }

        auto proc = factory (nullptr);
        ut->beginTest (proc->getName() + " Test");

        testFunc (proc.get());
    }
}

class UnitTests : public ConsoleApplication::Command
{
public:
    UnitTests();

    static void runUnitTests (const ArgumentList& args);

private:
    static int64 getRandomSeed (const ArgumentList& args);
    static Array<UnitTest*> getTestsForArgs (const ArgumentList& args);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnitTests)
};
