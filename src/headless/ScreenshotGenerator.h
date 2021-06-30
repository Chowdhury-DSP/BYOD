#pragma once

#include "../pch.h"

class ScreenshotGenerator : public ConsoleApplication::Command
{
public:
    ScreenshotGenerator();

private:
    /** Take a series of screenshots used for the plugin documentation */
    void takeScreenshots (const ArgumentList& args);

    /** Take a single screenshot for a given rectangle */
    void screenshotForBounds (Component* editor, Rectangle<int> bounds, const File& dir, const String& filename);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScreenshotGenerator)
};
