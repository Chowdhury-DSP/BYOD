#include "AmpIRs.h"

namespace
{
constexpr chowdsp::GlobalPluginSettings::SettingID userIRFolderID { "user_ir_folder" };
} // namespace

struct AmpIRsSelector : ComboBox, chowdsp::TrackedByBroadcasters
{
    AmpIRsSelector (AudioProcessorValueTreeState& vtState, AmpIRs& proc, chowdsp::HostContextProvider& hcp)
        : ampIRs (proc), vts (vtState)
    {
        pluginSettings->addProperties<&AmpIRsSelector::globalSettingChanged> ({ { userIRFolderID, String() } }, *this);

        refreshUserIRs();
        refreshBox();
        refreshText();
        this->setName (ampIRs.irTag + "__box");

        hcp.registerParameterComponent (*this, *vts.getParameter (ampIRs.irTag));
        onIRChanged = ampIRs.irChangedBroadcaster.connect ([this]
                                                           { refreshText(); });
        onChange = [this]
        { refreshText(); };

        setTextWhenNothingSelected ("Custom IR");
    }

    void visibilityChanged() override
    {
        setName (vts.getParameter (ampIRs.irTag)->name);
    }

    void globalSettingChanged (chowdsp::GlobalPluginSettings::SettingID settingID)
    {
        if (settingID != userIRFolderID)
            return;

        refreshUserIRs();
        refreshBox();
    }

    void refreshText()
    {
        setText (ampIRs.irState.name, sendNotification);
    }

    void refreshBox()
    {
        clear();
        auto* menu = getRootMenu();
        int menuIdx = 0;

        for (auto [idx, name] : chowdsp::enumerate (ampIRs.irNames))
        {
            if (name == "Custom")
                continue;

            PopupMenu::Item irItem;
            irItem.text = name;
            irItem.itemID = ++menuIdx;
            irItem.action = [this, index = (int) idx]
            {
                vts.getParameter (ampIRs.irTag)->setValueNotifyingHost ((float) index / (float) ampIRs.irNames.size());
            };
            menu->addItem (std::move (irItem));
        }

        if (! userIRFiles.empty())
            menu->addSubMenu ("User:", getUserIRsMenu());

        menu->addSeparator();

        PopupMenu::Item fromFileItem;
        fromFileItem.text = "Load From File";
        fromFileItem.itemID = ++menuIdx;
        fromFileItem.action = [this]
        { loadIRFromFile(); };
        menu->addItem (std::move (fromFileItem));

        PopupMenu::Item selectUserFolderItem;
        selectUserFolderItem.text = "Select User IRs Directory";
        selectUserFolderItem.itemID = ++menuIdx;
        selectUserFolderItem.action = [this]
        { selectUserIRsDirectory(); };
        menu->addItem (std::move (selectUserFolderItem));
    }

    void refreshUserIRs()
    {
        const auto userIRsFolder = File { pluginSettings->getProperty<String> (userIRFolderID) };
        if (! userIRsFolder.isDirectory())
            return;

        Logger::writeToLog ("Attempting to load user IRs from folder: " + userIRsFolder.getFullPathName());

        userIRFiles.clear();
        for (const DirectoryEntry& entry : RangedDirectoryIterator (userIRsFolder, true, ampIRs.audioFormatManager.getWildcardForAllFormats()))
            userIRFiles.emplace_back (entry.getFile());
    }

    PopupMenu getUserIRsMenu()
    {
        PopupMenu userIRsMenu;
        int menuIdx = 4321;
        for (const auto& irFile : userIRFiles)
        {
            if (! irFile.existsAsFile())
                continue;

            PopupMenu::Item fileItem;
            fileItem.text = irFile.getFileNameWithoutExtension();
            fileItem.itemID = menuIdx++;
            fileItem.action = [this, irFile]
            {
                ampIRs.loadIRFromStream (irFile.createInputStream(), {}, getTopLevelComponent());
            };
            userIRsMenu.addItem (fileItem);
        }

        return userIRsMenu;
    }

    void loadIRFromFile()
    {
        constexpr auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
        fileChooser = std::make_shared<FileChooser> ("Custom IR",
                                                     File(),
                                                     ampIRs.audioFormatManager.getWildcardForAllFormats(),
                                                     true,
                                                     false,
                                                     getTopLevelComponent());
        fileChooser->launchAsync (flags,
                                  [this, safeParent = SafePointer { getParentComponent() }] (const FileChooser& fc)
                                  {
#if JUCE_IOS
                                      if (fc.getURLResults().isEmpty())
                                          return;
                                      const auto irFile = fc.getURLResult();
                                      Logger::writeToLog ("AmpIRs attempting to load IR from local file: " + irFile.getLocalFile().getFullPathName());
                                      ampIRs.loadIRFromStream (irFile.createInputStream (URL::InputStreamOptions (URL::ParameterHandling::inAddress)),
                                                               {},
                                                               safeParent.getComponent());
#else
                if (fc.getResults().isEmpty())
                    return;
                const auto irFile = fc.getResult();

                Logger::writeToLog ("AmpIRs attempting to load IR from local file: " + irFile.getFullPathName());
                ampIRs.loadIRFromStream (irFile.createInputStream(), {}, safeParent.getComponent());
#endif
                                  });
    }

    void selectUserIRsDirectory()
    {
        constexpr auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;
        fileChooser = std::make_shared<FileChooser> ("User IR Folder", File(), "", true, false, getTopLevelComponent());
        fileChooser->launchAsync (flags,
                                  [this] (const FileChooser& fc)
                                  {
#if JUCE_IOS
                                      if (fc.getURLResults().isEmpty())
                                          return;
                                      const auto irFile = fc.getURLResult();
                                      pluginSettings->setProperty (userIRFolderID, irFile.getFullPathName());
#else
                if (fc.getResults().isEmpty())
                    return;
                const auto irFile = fc.getResult();
                pluginSettings->setProperty (userIRFolderID, irFile.getFullPathName());
#endif
                                  });
    }

    AmpIRs& ampIRs;
    AudioProcessorValueTreeState& vts;
    std::shared_ptr<FileChooser> fileChooser;
    chowdsp::ScopedCallback onIRChanged;

    chowdsp::SharedPluginSettings pluginSettings;
    std::vector<File> userIRFiles; // TODO: replace with a tree
};

bool AmpIRs::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    customComps.add (std::make_unique<AmpIRsSelector> (vts, *this, hcp));
    return false;
}
