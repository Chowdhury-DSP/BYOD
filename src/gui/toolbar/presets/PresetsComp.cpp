#include "PresetsComp.h"

PresetsComp::PresetsComp (PresetManager& presetMgr) : chowdsp::PresetsComp (presetMgr),
                                                      presetManager (presetMgr),
                                                      saveWindow (*this),
                                                      searchWindow (*this, presetMgr)
#if BYOD_BUILD_PRESET_SERVER
                                                      ,
                                                      loginWindow (*this),
                                                      syncWindow (*this)
#endif
{
    setNextPrevButton (Drawable::createFromImageData (BinaryData::RightArrow_svg, BinaryData::RightArrow_svgSize).get(), true);
    setNextPrevButton (Drawable::createFromImageData (BinaryData::LeftArrow_svg, BinaryData::LeftArrow_svgSize).get(), false);

    presetListUpdated();

#if BYOD_BUILD_PRESET_SERVER
    loginWindow.getViewComponent().loginChangeCallback = [&]
    { presetListUpdated(); };

    syncWindow.getViewComponent().runUpdateCallback = [&] (PresetManager::PresetUpdateList& presetUpdateList)
    { updatePresetsToUpdate (presetUpdateList); };
#endif

    saveWindow.getViewComponent().presetSaveCallback = [&] (const PresetSaveInfo& saveInfo)
    { savePreset (saveInfo); };
}

void PresetsComp::presetListUpdated()
{
    auto* menu = presetBox.getRootMenu();
    menu->clear();

    int optionID = 0;
    optionID = createPresetsMenu (optionID);
    menu->addSeparator();

    optionID = addBasicPresetOptions (menu, optionID);
    menu->addSeparator();

    optionID = addPresetShareOptions (menu, optionID);
    menu->addSeparator();

#if ! JUCE_IOS
    optionID = addCustomPresetFolderOptions (menu, optionID);
#endif

#if BYOD_BUILD_PRESET_SERVER
    addPresetServerMenuOptions (optionID);
#endif

    updatePresetBoxText();
}

#if BYOD_BUILD_PRESET_SERVER
void PresetsComp::syncServerPresetsToLocal()
{
    // user must have selected a preset path before trying to sync server presets
    if (presetManager.getUserPresetPath() == File())
    {
        chooseUserPresetFolder (
            [&]
            {
                if (presetManager.getUserPresetPath() != File())
                    syncServerPresetsToLocal();
            });
        return;
    }

    jobPool->addJob (
        [safeComp = Component::SafePointer (this)]
        {
            if (safeComp->presetManager.syncServerPresetsToLocal())
            {
                PresetsServerJobPool::callSafeOnMessageThread (safeComp,
                                                               [] (auto& c)
                                                               {
                                                                   c.syncWindow.getViewComponent().updatePresetsList (c.presetManager.getServerPresetUpdateList());
                                                                   c.syncWindow.show();
                                                               });
            }
        });
}

void PresetsComp::updatePresetsToUpdate (PresetManager::PresetUpdateList& presetUpdateList)
{
    const auto& userPresetPath = presetManager.getUserPresetPath();
    if (userPresetPath == File())
    {
        jassertfalse;
        return;
    }

    for (auto& [preset, _] : presetUpdateList)
        preset.toFile (presetManager.getPresetFile (preset));

    presetManager.loadUserPresetsFromFolder (userPresetPath);
}
#endif // BYOD_BUILD_PRESET_SERVER

int PresetsComp::createPresetsMenu (int optionID)
{
    struct VendorPresetCollection
    {
        std::map<juce::String, PopupMenu> categoryPresetMenus;
        std::vector<PopupMenu::Item> nonCategoryItems;
    };

    std::map<juce::String, VendorPresetCollection> presetMapItems;
    for (const auto& presetIDPair : manager.getPresetMap())
    {
        const auto presetID = presetIDPair.first;
        const auto& preset = presetIDPair.second;

        juce::PopupMenu::Item presetItem { preset.getName() };
        presetItem.itemID = presetID + 1;
        presetItem.action = [=, &preset]
        {
            updatePresetBoxText();
            manager.loadPreset (preset);
        };

        const auto& presetCategory = preset.getCategory();
        if (presetCategory.isEmpty()) // || presetCategory == "None")
            presetMapItems[preset.getVendor()].nonCategoryItems.push_back (presetItem);
        else
            presetMapItems[preset.getVendor()].categoryPresetMenus[presetCategory].addItem (presetItem);

        optionID = juce::jmax (optionID, presetItem.itemID);
    }

    for (auto& [vendorName, vendorCollection] : presetMapItems)
    {
        PopupMenu vendorMenu;
        for (auto& [category, categoryMenu] : vendorCollection.categoryPresetMenus)
            vendorMenu.addSubMenu (category, categoryMenu);

        std::sort (vendorCollection.nonCategoryItems.begin(), vendorCollection.nonCategoryItems.end(), [] (auto& item1, auto& item2)
                   { return item1.text < item2.text; });
        for (auto& extraItem : vendorCollection.nonCategoryItems)
            vendorMenu.addItem (extraItem);

        presetBox.getRootMenu()->addSubMenu (vendorName, vendorMenu);
    }

    return optionID;
}

int PresetsComp::addBasicPresetOptions (PopupMenu* menu, int optionID)
{
    optionID = addPresetMenuItem (menu,
                                  optionID,
                                  "Reset",
                                  [&]
                                  {
                                      if (auto* currentPreset = manager.getCurrentPreset())
                                          manager.loadPreset (*currentPreset);
                                  });

    optionID = addPresetMenuItem (menu,
                                  optionID,
                                  "Save Preset As",
                                  [&]
                                  {
                                      saveWindow.getViewComponent().prepareToShow();
                                      saveWindow.show();
                                  });

    // editing should only be allowed for user presets!
    if (presetManager.getCurrentPreset()->getVendor() == presetManager.getUserPresetName())
    {
        optionID = addPresetMenuItem (menu,
                                      optionID,
                                      "Resave Preset",
                                      [&]
                                      {
                                          if (auto* currentPreset = manager.getCurrentPreset())
                                          {
                                              auto presetFile = currentPreset->getPresetFile();
                                              if (presetFile == File())
                                                  presetFile = presetManager.getPresetFile (*currentPreset);

                                              saveWindow.getViewComponent().prepareToShow (currentPreset, presetFile);
                                              saveWindow.show();
                                          }
                                      });
    }

    if (presetManager.getCurrentPreset()->getPresetFile() != File())
    {
        optionID = addPresetMenuItem (menu,
                                      optionID,
                                      "Delete Preset",
                                      [&]
                                      {
                                          if (auto* currentPreset = manager.getCurrentPreset())
                                          {
                                              auto presetFile = currentPreset->getPresetFile();
                                              if (! (presetFile.existsAsFile() && presetFile.hasFileExtension (PresetConstants::presetExt)))
                                              {
                                                  NativeMessageBox::showMessageBox (MessageBoxIconType::WarningIcon, "Preset Deletion", "Unable to find preset file!");
                                                  return;
                                              }

                                              if (NativeMessageBox::showOkCancelBox (MessageBoxIconType::QuestionIcon, "Preset Deletion", "Are you sure you want to delete this preset? This operation cannot be undone."))
                                              {
                                                  presetFile.deleteFile();
                                                  manager.loadDefaultPreset();
                                                  manager.loadUserPresetsFromFolder (manager.getUserPresetPath());
                                              }
                                          }
                                      });
    }

    return addPresetMenuItem (menu, optionID, "Search", [&]
                              { searchWindow.show(); });
}

int PresetsComp::addPresetShareOptions (PopupMenu* menu, int optionID)
{
    optionID = addPresetMenuItem (menu,
                                  optionID,
                                  "Copy Current Preset",
                                  [&]
                                  {
                                      if (auto* currentPreset = manager.getCurrentPreset())
                                          SystemClipboard::copyTextToClipboard (currentPreset->toXml()->toString());
                                  });

    optionID = addPresetMenuItem (menu,
                                  optionID,
                                  "Paste Preset",
                                  [&]
                                  {
                                      const auto presetText = SystemClipboard::getTextFromClipboard();
                                      if (presetText.isEmpty())
                                          return;

                                      if (auto presetXml = XmlDocument::parse (presetText))
                                          presetManager.loadPresetSafe (std::make_unique<chowdsp::Preset> (presetXml.get()));
                                  });

#if ! JUCE_IOS
    return addPresetMenuItem (menu, optionID, "Load Preset From File", [&]
                              { loadFromFileBrowser(); });
#else
    return optionID;
#endif
}

int PresetsComp::addCustomPresetFolderOptions (PopupMenu* menu, int optionID)
{
    if (manager.getUserPresetPath().isDirectory())
    {
        optionID = addPresetMenuItem (menu, optionID, "Go to Preset Folder...", [&]
                                      { manager.getUserPresetPath().startAsProcess(); });
    }

    return addPresetMenuItem (menu, optionID, "Choose Preset Folder...", [&]
                              { chooseUserPresetFolder ({}); });
}

void PresetsComp::loadFromFileBrowser()
{
    constexpr auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
    fileChooser = std::make_shared<FileChooser> ("Load Preset", manager.getUserPresetPath(), "*" + PresetConstants::presetExt, true, false, getTopLevelComponent());
    fileChooser->launchAsync (flags,
                              [&] (const FileChooser& fc)
                              {
                                  if (fc.getResults().isEmpty())
                                      return;

                                  presetManager.loadPresetSafe (std::make_unique<chowdsp::Preset> (fc.getResult()));
                              });
}

#if BYOD_BUILD_PRESET_SERVER
int PresetsComp::addPresetServerMenuOptions (int optionID)
{
    auto menu = presetBox.getRootMenu();
    menu->addSeparator();

    if (userManager->getIsLoggedIn())
    {
        menu->addSectionHeader ("Logged in as: " + userManager->getUsername());

        PopupMenu::Item syncLocalToServerItem { "Sync Local -> Server" };
        syncLocalToServerItem.itemID = ++optionID;
        syncLocalToServerItem.action = [=]
        {
            updatePresetBoxText();
            presetManager.syncLocalPresetsToServer();
        };
        menu->addItem (syncLocalToServerItem);

        PopupMenu::Item syncServerToLocalItem { "Sync Server -> Local" };
        syncServerToLocalItem.itemID = ++optionID;
        syncServerToLocalItem.action = [=]
        {
            updatePresetBoxText();
            syncServerPresetsToLocal();
        };
        menu->addItem (syncServerToLocalItem);

        PopupMenu::Item loginItem { "Log Out" };
        loginItem.itemID = ++optionID;
        loginItem.action = [=]
        {
            updatePresetBoxText();
            userManager->logOut();
            presetListUpdated();
        };
        menu->addItem (loginItem);
    }
    else
    {
        PopupMenu::Item loginItem { "Login" };
        loginItem.itemID = ++optionID;
        loginItem.action = [=]
        {
            updatePresetBoxText();
            loginWindow.show();
        };
        menu->addItem (loginItem);
    }

    return optionID;
}
#endif // BYOD_BUILD_PRESET_SERVER

void PresetsComp::savePreset (const PresetSaveInfo& saveInfo)
{
    auto savePresetLambda = [] (PresetManager& presetMgr, const PresetSaveInfo& sInfo)
    {
        if (presetMgr.getPresetFile (presetMgr.getUserPresetName(), sInfo.category, sInfo.name).existsAsFile())
        {
            const String warningBoxTitle = "Preset Save Warning!";
            const String warningBoxMessage = "You are about to overwrite an existing preset! Are you sure you want to continue?";
            if (NativeMessageBox::showYesNoBox (MessageBoxIconType::WarningIcon, warningBoxTitle, warningBoxMessage) == 0)
                return;
        }

        presetMgr.saveUserPreset (sInfo.name, sInfo.category, sInfo.isPublic, sInfo.presetID);
    };

    auto presetPath = manager.getUserPresetPath();
    if (presetPath == juce::File() || ! presetPath.isDirectory())
    {
        presetPath.deleteRecursively();
        chooseUserPresetFolder ([&, saveInfo = saveInfo]
                                { savePresetLambda (presetManager, saveInfo); });
    }
    else
    {
        savePresetLambda (presetManager, saveInfo);
    }
}

void PresetsComp::selectedPresetChanged()
{
    presetListUpdated();
}
