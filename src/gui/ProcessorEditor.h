#pragma once

#include "../processors/BaseProcessor.h"

class ProcessorEditor : public Component
{
public:
    ProcessorEditor (BaseProcessor& baseProc);

    void paint (Graphics& g) override;

private:
    BaseProcessor& proc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
};
