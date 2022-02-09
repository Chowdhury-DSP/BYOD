#include "PresetsComp.h"

PresetsComp::PresetsComp (PresetManager& presetMgr) : chowdsp::PresetsComp (presetMgr),
                                                      presetManager (presetMgr),
                                                      loginWindow (*this),
                                                      saveWindow (*this),
                                                      searchWindow (*this, presetMgr),
                                                      syncWindow (*this)
{
    presetListUpdated();

    loginWindow.getViewComponent().loginChangeCallback = [&]
    { presetListUpdated(); };

    syncWindow.getViewComponent().runUpdateCallback = [&]
    { updatePresetsToUpdate(); };

    saveWindow.getViewComponent().presetSaveCallback = [&] (const PresetSaveInfo& saveInfo)
    { savePreset (saveInfo); };
}

void PresetsComp::presetListUpdated()
{
    presetBox.getRootMenu()->clear();

    int optionID = 0;
    optionID = createPresetsMenu (optionID);
    optionID = addPresetOptions (optionID);
    addPresetServerMenuOptions (optionID);
}

void PresetsComp::syncServerPresetsToLocal()
{
    // user must have selected a preset path before trying to sync server presets
    if (presetManager.getUserPresetPath() == File())
    {
        chooseUserPresetFolder ([&]
                                { syncServerPresetsToLocal(); });
        return;
    }

    presetsToUpdate.clear();
    presetManager.syncServerPresetsToLocal (presetsToUpdate);

    syncWindow.getViewComponent().updatePresetsList (presetsToUpdate);
    syncWindow.show();
}

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

void PresetsComp::updatePresetsToUpdate()
{
    const auto& userPresetPath = presetManager.getUserPresetPath();
    if (userPresetPath == File())
    {
        jassertfalse;
        return;
    }

    for (auto& [preset, _] : presetsToUpdate)
        preset.toFile (userPresetPath.getChildFile (preset.getName() + PresetConstants::presetExt));

    presetManager.loadUserPresetsFromFolder (userPresetPath);
}

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

int PresetsComp::addPresetOptions (int optionID)
{
    auto menu = presetBox.getRootMenu();
    menu->addSeparator();

    juce::PopupMenu::Item saveItem { "Save Preset" };
    saveItem.itemID = ++optionID;
    saveItem.action = [=]
    {
        updatePresetBoxText();
        saveWindow.getViewComponent().prepareToShow();
        saveWindow.show();
    };
    menu->addItem (saveItem);

    // editing should only be allowed for user presets!
    if (presetManager.getCurrentPreset()->getVendor() == presetManager.getUserPresetName())
    {
        juce::PopupMenu::Item editItem { "Edit Preset" };
        editItem.itemID = ++optionID;
        editItem.action = [&]
        {
            if (auto* currentPreset = manager.getCurrentPreset())
            {
                auto presetFile = currentPreset->getPresetFile();
                if (presetFile == File())
                    presetFile = presetManager.getPresetFile (*currentPreset);

                updatePresetBoxText();
                saveWindow.getViewComponent().prepareToShow (currentPreset, presetFile);
                saveWindow.show();
            }
        };
        menu->addItem (editItem);
    }

    juce::PopupMenu::Item searchItem { "Search" };
    searchItem.itemID = ++optionID;
    searchItem.action = [&]
    {
        searchWindow.show();
    };
    menu->addItem (searchItem);

#if ! JUCE_IOS
    juce::PopupMenu::Item goToFolderItem { "Go to Preset folder..." };
    goToFolderItem.itemID = ++optionID;
    goToFolderItem.action = [=]
    {
        updatePresetBoxText();
        auto folder = manager.getUserPresetPath();
        if (folder.isDirectory())
            folder.startAsProcess();
        else
            chooseUserPresetFolder ({});
    };
    menu->addItem (goToFolderItem);

    juce::PopupMenu::Item chooseFolderItem { "Choose Preset folder..." };
    chooseFolderItem.itemID = ++optionID;
    chooseFolderItem.action = [=]
    {
        updatePresetBoxText();
        chooseUserPresetFolder ({});
    };
    menu->addItem (chooseFolderItem);
#endif

    return optionID;
}

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

void PresetsComp::selectedPresetChanged()
{
    presetListUpdated();
}
