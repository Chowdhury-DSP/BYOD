#include "SettingsButton.h"
#include "gui/BoardViewport.h"
#include "processors/chain/ProcessorChainPortMagnitudesHelper.h"

namespace
{
const Colour onColour = Colours::yellow;
const Colour offColour = Colours::white;
} // namespace

SettingsButton::SettingsButton (const AudioProcessor& processor) : DrawableButton ("Settings", DrawableButton::ImageFitted),
                                                                   proc (processor)
{
    auto cog = Drawable::createFromImageData (BinaryData::cogsolid_svg, BinaryData::cogsolid_svgSize);
    setImages (cog.get());

    onClick = [=]
    { showSettingsMenu(); };
}

SettingsButton::~SettingsButton() = default;

void SettingsButton::showSettingsMenu()
{
    PopupMenu menu;

    cableVizMenu (menu, 100);
    defaultZoomMenu (menu, 200);

    menu.addSeparator();
    menu.addItem ("Copy Diagnostic Info", [=]
                  { copyDiagnosticInfo(); });

    auto options = PopupMenu::Options()
                       .withParentComponent (getTopLevelComponent())
                       .withPreferredPopupDirection (PopupMenu::Options::PopupDirection::upwards)
                       .withStandardItemHeight (27);
    menu.showMenuAsync (options);
}

void SettingsButton::cableVizMenu (PopupMenu& menu, int itemID)
{
    const auto isCurrentlyOn = (bool) pluginSettings->getProperty (ProcessorChainPortMagnitudesHelper::cableVizOnOffID);

    PopupMenu::Item item;
    item.itemID = ++itemID;
    item.text = "Cable Visualizations";
    item.action = [=]
    { pluginSettings->setProperty (ProcessorChainPortMagnitudesHelper::cableVizOnOffID, ! isCurrentlyOn); };
    item.colour = isCurrentlyOn ? onColour : offColour;

    menu.addItem (item);
}

void SettingsButton::defaultZoomMenu (PopupMenu& menu, int itemID)
{
    PopupMenu defaultZoomMenu;

    const auto curDefaultZoomLevel = (double) pluginSettings->getProperty (BoardViewport::defaultZoomSettingID);
    for (auto zoomExp : { -4, -3, -2, -1, 0, 1, 2, 3, 4 })
    {
        auto zoomLevel = std::pow (1.1, (double) zoomExp);

        PopupMenu::Item item;
        item.itemID = ++itemID;
        item.text = String (int (zoomLevel * 100.0)) + "%";
        item.action = [=]
        { pluginSettings->setProperty (BoardViewport::defaultZoomSettingID, zoomLevel); };
        item.colour = isWithin (zoomLevel, curDefaultZoomLevel, 0.001) ? onColour : offColour;

        defaultZoomMenu.addItem (item);
    }

    menu.addSubMenu ("Default Zoom", defaultZoomMenu);
}

void SettingsButton::copyDiagnosticInfo()
{
    Logger::writeToLog ("Copying diagnostic info...");

    String diagString;
    diagString += "Version: " + proc.getName() + " " + String (JucePlugin_VersionString) + "\n";
    diagString += "Commit: " + String (BYOD_GIT_COMMIT_HASH) + " on " + String (BYOD_GIT_BRANCH)
                  + " with JUCE version " + SystemStats::getJUCEVersion() + "\n";

    // build system info
    diagString += "Build: " + Time::getCompilationDate().toString (true, true, false, true)
                  + " on " + String (BYOD_BUILD_FQDN)
                  + " with " + String (BYOD_CXX_COMPILER_ID) + "-" + String (BYOD_CXX_COMPILER_VERSION) + "\n";

    // user system info
    diagString += "System: " + SystemStats::getDeviceDescription()
                  + " with " + SystemStats::getOperatingSystemName()
                  + (SystemStats::isOperatingSystem64Bit() ? " (64-bit)" : String())
                  + (SystemStats::isRunningInAppExtensionSandbox() ? " (Sandboxed)" : String())
                  + " on " + String (SystemStats::getNumCpus()) + " Core, " + SystemStats::getCpuModel() + "\n";

    // plugin info
    PluginHostType hostType {};
    diagString += "Plugin Info: " + String (AudioProcessor::getWrapperTypeDescription (proc.wrapperType))
                  + " running in " + String (hostType.getHostDescription())
                  + " running at " + String (proc.getSampleRate() / 1000.0, 1) + " kHz "
                  + "with block size: " + String (proc.getBlockSize()) + "\n";

    SystemClipboard::copyTextToClipboard (diagString);
}
