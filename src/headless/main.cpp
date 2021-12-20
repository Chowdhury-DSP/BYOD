#include "../pch.h"
#include "PresetResaver.h"
#include "PresetSaveLoadTime.h"
#include "ScreenshotGenerator.h"
#include "tests/UnitTests.h"

String getVersion()
{
    return String (ProjectInfo::projectName) + " - " + ProjectInfo::versionString;
}

String getHelp()
{
    return "BYOD Headless Interface:";
}

int main (int argc, char* argv[])
{
    std::cout << "Running BYOD in headless mode..." << std::endl;

#if JUCE_MAC
    Process::setDockIconVisible (false); // hide dock icon
#endif
    ScopedJuceInitialiser_GUI scopedJuce; // creates MessageManager

    ConsoleApplication app;
    app.addVersionCommand ("--version", getVersion());
    app.addHelpCommand ("--help|-h", getHelp(), true);

    ScreenshotGenerator screenshooter;
    app.addCommand (ScreenshotGenerator());

    PresetResaver presetsResaver;
    app.addCommand (presetsResaver);

    PresetSaveLoadTime presetSaveLoadTime;
    app.addCommand (presetSaveLoadTime);

    UnitTests unitTests;
    app.addCommand (unitTests);

    // ArgumentList args { "--unit-tests", "--all" };
    // unitTests.runUnitTests (args);

    return app.findAndRunCommand (argc, argv);
}
