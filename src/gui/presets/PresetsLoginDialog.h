#pragma once

#include "ServerWaitingSpinner.h"
#include "state/presets/PresetsServerJobPool.h"
#include "state/presets/PresetsServerUserManager.h"

class PresetsLoginDialog : public Component
{
public:
    PresetsLoginDialog();

    void visibilityChanged() override;

    void resized() override;
    void paint (Graphics& g) override { g.fillAll (Colours::black); }

    std::function<void()> loginChangeCallback = [] {};

private:
    void doRegisterLoginAction (bool registerAction = false);

    TextEditor username, password;
    TextButton loginButton { "Login" }, cancelButton { "Cancel" }, registerButton { "Register" };

    ServerWaitingSpinner spinner;
    SharedPresetsServerUserManager userManager;

    SharedPresetsServerJobPool jobPool;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsLoginDialog)
};
