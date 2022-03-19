#include "SettingsButton.h"
#include "SystemInfo.h"
#include "gui/BoardViewport.h"
#include "processors/chain/ProcessorChainPortMagnitudesHelper.h"

namespace
{
const Colour onColour = Colours::yellow;
const Colour offColour = Colours::white;
} // namespace

SettingsButton::SettingsButton (const BYOD& processor, chowdsp::OpenGLHelper* oglHelper) : DrawableButton ("Settings", DrawableButton::ImageFitted),
                                                                                           proc (processor),
                                                                                           openGLHelper (oglHelper)
{
    Logger::writeToLog ("Checking OpenGL availability...");
    const auto shouldUseOpenGLByDefault = openGLHelper != nullptr && openGLHelper->isOpenGLAvailable();
    Logger::writeToLog ("OpenGL is available on this system: " + String (shouldUseOpenGLByDefault ? "TRUE" : "FALSE"));
    pluginSettings->addProperties ({ { openglID, shouldUseOpenGLByDefault } }, this);
    globalSettingChanged (openglID);

    auto cog = Drawable::createFromImageData (BinaryData::cogsolid_svg, BinaryData::cogsolid_svgSize);
    setImages (cog.get());

    onClick = [=]
    { showSettingsMenu(); };
}

SettingsButton::~SettingsButton()
{
    pluginSettings->removePropertyListener (this);
}

void SettingsButton::globalSettingChanged (SettingID settingID)
{
    if (settingID != openglID)
        return;

    if (openGLHelper != nullptr)
    {
        const auto shouldUseOpenGL = pluginSettings->getProperty<bool> (openglID);
        if (shouldUseOpenGL == openGLHelper->isAttached())
            return; // no change

        Logger::writeToLog ("Using OpenGL: " + String (shouldUseOpenGL ? "TRUE" : "FALSE"));
        shouldUseOpenGL ? openGLHelper->attach() : openGLHelper->detach();
    }
}

void SettingsButton::showSettingsMenu()
{
    PopupMenu menu;

    cableVizMenu (menu, 100);
    defaultZoomMenu (menu, 200);
    openGLManu (menu, 300);

    menu.addSeparator();
    menu.addItem ("View Source Code", [=]
                  { URL ("https://github.com/Chowdhury-DSP/BYOD").launchInDefaultBrowser(); });

    menu.addItem ("Copy Diagnostic Info", [=]
                  { copyDiagnosticInfo(); });

    // get top level component that is big enough
    Component* parentComp = this;
    int iters = 0;
    while (true)
    {
        if (parentComp->getWidth() > 80 && parentComp->getHeight() > 100)
            break;

        parentComp = parentComp->getParentComponent();

        if (iters > 5 || parentComp == nullptr)
        {
            jassertfalse; // unable to find component large enough!
            return;
        }
    }

    auto options = PopupMenu::Options()
                       .withParentComponent (parentComp)
                       .withPreferredPopupDirection (PopupMenu::Options::PopupDirection::upwards)
                       .withStandardItemHeight (27);
    menu.setLookAndFeel (lnfAllocator->getLookAndFeel<ByodLNF>());
    menu.showMenuAsync (options);
}

void SettingsButton::cableVizMenu (PopupMenu& menu, int itemID)
{
    const auto isCurrentlyOn = pluginSettings->getProperty<bool> (ProcessorChainPortMagnitudesHelper::cableVizOnOffID);

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

    const auto curDefaultZoomLevel = pluginSettings->getProperty<double> (BoardViewport::defaultZoomSettingID);
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

void SettingsButton::openGLManu (PopupMenu& menu, int itemID)
{
    if (openGLHelper == nullptr || ! openGLHelper->isOpenGLAvailable())
        return;

    const auto isCurrentlyOn = pluginSettings->getProperty<bool> (openglID);

    PopupMenu::Item item;
    item.itemID = ++itemID;
    item.text = "Use OpenGL";
    item.action = [=]
    { pluginSettings->setProperty (openglID, ! isCurrentlyOn); };
    item.colour = isCurrentlyOn ? onColour : offColour;

    menu.addItem (item);
}

void SettingsButton::copyDiagnosticInfo()
{
    Logger::writeToLog ("Copying diagnostic info...");
    SystemClipboard::copyTextToClipboard (SystemInfo::getDiagnosticsString (proc));
}
