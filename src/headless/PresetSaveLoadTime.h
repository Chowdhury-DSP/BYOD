#pragma once

#include "../pch.h"

class PresetSaveLoadTime : public ConsoleApplication::Command
{
public:
    PresetSaveLoadTime();

private:
    static void timePresetSaveAndLoad (const ArgumentList& args);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetSaveLoadTime)
};
