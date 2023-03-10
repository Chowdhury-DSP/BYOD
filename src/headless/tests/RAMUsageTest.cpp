#include "UnitTests.h"

#if JUCE_MAC
// borrowed from: https://developer.apple.com/forums/thread/105088?answerId=357415022#357415022
static float getCurrentMemoryUsageGB()
{
    auto info = task_vm_info_data_t {};
    auto info_count = TASK_VM_INFO_COUNT;

    if (task_info (mach_task_self(), task_flavor_t (TASK_VM_INFO), reinterpret_cast<task_info_t> (&info), &info_count) == KERN_SUCCESS)
    {
        return float (info.phys_footprint) * 1.0e-9f; // phys_footprint is in bytes, so let's convert to GigaBytes
    }

    // unable to get virtual memory info on this platform!
    return 0.0f;
}

class RAMUSageTest : public UnitTest
{
public:
    RAMUSageTest() : UnitTest ("RAM Usage Test")
    {
    }

    void runTest() override
    {
        struct UsageInfo
        {
            std::string name;
            float usageGB = 0.0f;
        };
        std::vector<UsageInfo> usageInfo;
        usageInfo.reserve (ProcessorStore::getStoreMap().size());

        const auto baseMemoryUsage = getCurrentMemoryUsageGB();
        runTestForAllProcessors (
            this,
            [baseMemoryUsage, &usageInfo] (BaseProcessor* proc)
            {
                proc->prepareProcessing (48000.0, 1024);
                const auto memoryUsage = getCurrentMemoryUsageGB() - baseMemoryUsage;
                usageInfo.push_back ({ proc->getName().toStdString(), memoryUsage });
            });

        std::sort (usageInfo.begin(), usageInfo.end(), [] (const auto& u1, const auto& u2)
                   { return u1.usageGB < u2.usageGB; });
        for (const auto& uInfo : usageInfo)
            std::cout << uInfo.name << ": " << uInfo.usageGB << std::endl;
    }
};

static RAMUSageTest ramUsageTest;
#endif
