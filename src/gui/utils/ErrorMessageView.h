#pragma once

#include <pch.h>

class ErrorMessageView : public Component
{
public:
    ErrorMessageView();

    static void showErrorMessage (const String& title, const String& message, const String& buttonText, Component* comp);

    /** Returns true if the answer is yes */
    static bool showYesNoBox (const String& title, const String& message, Component* comp);

    static void showCustomMessage (const String& title,
                                   const String& message,
                                   const String& button1Text,
                                   std::function<void()>&& button1Action,
                                   const String& button2Text,
                                   std::function<void()>&& button2Action,
                                   Component* comp);

    static void showCrashLogView (File logFile, Component* comp);

    void setParameters (const String& title, const String& message, const String& buttonText);
    void setParametersYesNo (const String& title, const String& message);
    void setParametersCustom (const String& title,
                              const String& message,
                              const String& button1Text,
                              std::function<void()>&& button1Action,
                              const String& button2Text,
                              std::function<void()>&& button2Action);

    void paint (Graphics& g) override;
    void resized() override;

private:
    Label title;
    Label message;
    TextButton closeButton;
    TextButton yesButton { "YES" };
    TextButton noButton { "NO" };
    TextButton custom1Button { "" };
    TextButton custom2Button { "" };
    int result = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ErrorMessageView)
};
