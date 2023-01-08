#include "SettingsButton.h"
#include "BYOD.h"
#include "gui/pedalboard/BoardViewport.h"
#include "processors/chain/ProcessorChainPortMagnitudesHelper.h"
#include "state/ParamForwardManager.h"

namespace
{
const Colour onColour = Colours::yellow;
const Colour offColour = Colours::white;
} // namespace

SettingsButton::SettingsButton (BYOD& processor, chowdsp::OpenGLHelper* oglHelper) : DrawableButton ("Settings", DrawableButton::ImageFitted),
                                                                                     proc (processor),
                                                                                     openGLHelper (oglHelper)
#if BYOD_ENABLE_ADD_ON_MODULES
                                                                                     ,
                                                                                     modulePacksDialog (*this, processor.getPresetManager())
#endif
{
    Logger::writeToLog ("Checking OpenGL availability...");
    const auto shouldUseOpenGLByDefault = openGLHelper != nullptr && openGLHelper->isOpenGLAvailable();
#if CHOWDSP_OPENGL_IS_AVAILABLE
    Logger::writeToLog ("OpenGL is available on this system: " + String (shouldUseOpenGLByDefault ? "TRUE" : "FALSE"));
#else
    Logger::writeToLog ("Plugin was built without linking to OpenGL!");
#endif
    pluginSettings->addProperties<&SettingsButton::globalSettingChanged> ({ { openglID, shouldUseOpenGLByDefault } }, *this);
    globalSettingChanged (openglID);

    setImages (Drawable::createFromImageData (BinaryData::cogsolid_svg, BinaryData::cogsolid_svgSize).get());
    onClick = [this]
    { showSettingsMenu(); };
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

    addPluginSettingMenuOption ("Cable Visualizations", ProcessorChainPortMagnitudesHelper::cableVizOnOffID, menu, 100);
    
    if (openGLHelper != nullptr && openGLHelper->isOpenGLAvailable())
        addPluginSettingMenuOption ("Use OpenGL", openglID, menu, 200);
    
    if (pluginSettings->hasProperty (ParamForwardManager::refreshParamTreeID))
        addPluginSettingMenuOption ("Refresh Parameter Tree", ParamForwardManager::refreshParamTreeID, menu, 300);
    
    defaultZoomMenu (menu, 400);

    menu.addSeparator();
    menu.addItem ("View Source Code", []
                  { URL ("https://github.com/Chowdhury-DSP/BYOD").launchInDefaultBrowser(); });

    menu.addItem ("Copy Diagnostic Info", [this]
                  { copyDiagnosticInfo(); });

#if BYOD_ENABLE_ADD_ON_MODULES
    menu.addSeparator();
    menu.addItem ("Purchase Add-On Modules", [this]
                  { modulePacksDialog.show(); });
#endif

    auto options = PopupMenu::Options()
                       .withParentComponent (getParentComponent()->getParentComponent()) // BYODPluginEditor
                       .withPreferredPopupDirection (PopupMenu::Options::PopupDirection::upwards)
                       .withStandardItemHeight (27);
    menu.setLookAndFeel (lnfAllocator->getLookAndFeel<ByodLNF>());
    menu.showMenuAsync (options);
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
        item.action = [this, zoomLevel]
        { pluginSettings->setProperty (BoardViewport::defaultZoomSettingID, zoomLevel); };
        item.colour = isWithin (zoomLevel, curDefaultZoomLevel, 0.001) ? onColour : offColour;

        defaultZoomMenu.addItem (item);
    }

    menu.addSubMenu ("Default Zoom", defaultZoomMenu);
}

void SettingsButton::copyDiagnosticInfo()
{
    Logger::writeToLog ("Copying diagnostic info...");
    SystemClipboard::copyTextToClipboard (chowdsp::PluginDiagnosticInfo::getDiagnosticsString (proc));
}

void SettingsButton::addPluginSettingMenuOption (const String& name, const SettingID& id, PopupMenu& menu, int itemID)
{
    const auto isCurrentlyOn = pluginSettings->getProperty<bool> (id);

    PopupMenu::Item item;
    item.itemID = itemID;
    item.text = name;
    item.action = [this, id, isCurrentlyOn]
    { pluginSettings->setProperty (id, ! isCurrentlyOn); };
    item.colour = isCurrentlyOn ? onColour : offColour;

    menu.addItem (item);
}
