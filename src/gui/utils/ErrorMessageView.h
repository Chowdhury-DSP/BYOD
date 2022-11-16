#pragma once

#include <pch.h>

class ErrorMessageView : public Component
{
public:
    ErrorMessageView();

    static void showErrorMessage (const String& title, const String& message, const String& buttonText, Component* comp);

    /** Returns true if the answer is yes */
    static bool showYesNoBox (const String& title, const String& message, Component* comp);

    void setParameters (const String& title, const String& message, const String& buttonText);
    void setParametersYesNo (const String& title, const String& message);

    void paint (Graphics& g) override;
    void resized() override;

private:
    Label title;
    Label message;
    TextButton closeButton;
    TextButton yesButton { "YES" };
    TextButton noButton { "NO" };
    int result = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ErrorMessageView)
};
