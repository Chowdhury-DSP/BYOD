#include "PresetsComp.h"

PresetsComp::PresetsComp (PresetManager& presetMgr) : chowdsp::PresetsComp (presetMgr),
                                                      presetManager (presetMgr),
                                                      loginWindow (*this)
{
    presetListUpdated();

    auto& loginWindowComp = loginWindow.getViewComponent();
    loginWindowComp.loginChangeCallback = [&]
    { presetListUpdated(); };
}

void PresetsComp::presetListUpdated()
{
    presetBox.getRootMenu()->clear();

    int optionID = 0;
    optionID = createPresetsMenu (optionID);
    optionID = addPresetOptions (optionID);
    addPresetServerMenuOptions (optionID);
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
            presetManager.syncServerPresetsToLocal();
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
