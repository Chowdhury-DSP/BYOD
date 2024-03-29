#pragma once

#include "../pch.h"

class ScreenshotGenerator : public ConsoleApplication::Command
{
public:
    ScreenshotGenerator();

private:
    /** Take a series of screenshots used for the plugin documentation */
    static void takeScreenshots (const ArgumentList& args);

    /** Take a single screenshot for a given rectangle */
    static void screenshotForBounds (Component* editor, juce::Rectangle<int> bounds, const File& dir, const String& filename);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScreenshotGenerator)
};
