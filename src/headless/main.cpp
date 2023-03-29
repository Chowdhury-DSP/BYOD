#include "GuitarMLFilterDesigner.h"
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

    app.addCommand (ScreenshotGenerator());
    app.addCommand (PresetResaver());
    app.addCommand (PresetSaveLoadTime());
    app.addCommand (GuitarMLFilterDesigner());
    app.addCommand (UnitTests());

    // ArgumentList args { "--unit-tests", "--all" };
    // unitTests.runUnitTests (args);

    return app.findAndRunCommand (argc, argv);
}
