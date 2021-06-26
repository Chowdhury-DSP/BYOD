#pragma once

#include "processors/BaseProcessor.h"

class InfoComponent : public Component
{
public:
    InfoComponent();

    void paint (Graphics& g) override;
    void resized() override;

    void setInfoForProc (const String& name, const ProcessorUIOptions::ProcInfo& info);

private:
    TextButton xButton;
    String procName, authors, description;
    int numAuthors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InfoComponent)
};
