#if BYOD_BUILD_PRESET_SERVER

#pragma once

#include <pch.h>

struct PresetsServerJobPool
{
    ~PresetsServerJobPool()
    {
        pool.removeAllJobs (true, 250);
    }

    template <typename JobType>
    void addJob (JobType&& job)
    {
        pool.addJob (job);
    }

    /** Safely run an action with a Component::SafePointer object (no return value) */
    template <typename SafeCompType, typename ActionType>
    static void callSafeOnMessageThread (SafeCompType& safeComp, ActionType&& action)
    {
        MessageManager::callAsync (
            [safeComp, &action]
            {
                if (auto* c = safeComp.getComponent())
                    action (*c);
            });
    }

private:
    ThreadPool pool { 2 };
};

using SharedPresetsServerJobPool = SharedResourcePointer<PresetsServerJobPool>;

#endif // BYOD_BUILD_PRESET_SERVER
