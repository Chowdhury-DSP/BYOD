#pragma once

#include "../pch.h"

class GuitarMLFilterDesigner : public ConsoleApplication::Command
{
public:
    GuitarMLFilterDesigner();

private:
    /** Generates files for plotting GuitarML frequency content */
    static void generateFiles (const ArgumentList& args);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarMLFilterDesigner)
};