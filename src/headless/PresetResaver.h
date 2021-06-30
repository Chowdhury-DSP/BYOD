#pragma once

#include "../pch.h"

class PresetResaver : public ConsoleApplication::Command
{
public:
    PresetResaver();

private:
    /** Re-saves the plugin presets with correct state */
    void savePresets (const ArgumentList& args);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetResaver)
};
