#pragma once

#include "processors/BaseProcessor.h"

struct Cable
{
    Cable (BaseProcessor* start, int startPort) : startProc (start),
                                                  startIdx (startPort)
    {
    }

    Cable (BaseProcessor* start, int startPort, BaseProcessor* end, int endPort) : startProc (start),
                                                                                   startIdx (startPort),
                                                                                   endProc (end),
                                                                                   endIdx (endPort)
    {
    }

    BaseProcessor* startProc = nullptr;
    int startIdx;

    BaseProcessor* endProc = nullptr;
    int endIdx = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cable)
};
