#pragma once

#include "../processors/BaseProcessor.h"

struct Cable
{
    Cable (BaseProcessor* start, int startPort) : startProc (start),
                                                  startIdx (startPort)
    {
    }

    BaseProcessor* startProc = nullptr;
    int startIdx;

    BaseProcessor* endProc = nullptr;
    int endIdx = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cable)
};
