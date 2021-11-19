#pragma once

#include "BoardComponent.h"

class BoardMessageManager : public Timer
{
public:
    BoardMessageManager() = default;

    using MessageType = std::function<void()>;

    void timerCallback() override
    {
        ScopedLock sl (crit);
        
        while (! messageBuffer.empty())
        {
            messageBuffer.front()();
            messageBuffer.pop();
        }
    }

    void pushMessage (MessageType&& message)
    {
        // not worth deferring if we're already on this thread!
        if (MessageManager::existsAndIsCurrentThread())
        {
            message();
            return;
        }

        ScopedLock sl (crit);
        messageBuffer.push (std::move (message));
    }

private:
    std::queue<MessageType> messageBuffer;
    
    CriticalSection crit;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardMessageManager)
};
