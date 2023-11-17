#pragma once

#include <juce_core/juce_core.h>

namespace jai
{
struct Context;
}
struct JaiContextWrapper
{
    JaiContextWrapper();
    ~JaiContextWrapper();

    operator jai::Context*() { return internal; }; // NOLINT

private:
    jai::Context* internal = nullptr;
};

using SharedJaiContext = juce::SharedResourcePointer<JaiContextWrapper>;
