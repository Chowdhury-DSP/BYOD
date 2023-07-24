#pragma once

#include <juce_core/juce_core.h>

struct jai_Context;
struct JaiContextWrapper
{
    JaiContextWrapper();
    ~JaiContextWrapper();

    operator jai_Context*() { return internal; }; // NOLINT

private:
    jai_Context* internal = nullptr;
};

using SharedJaiContext = juce::SharedResourcePointer<JaiContextWrapper>;
