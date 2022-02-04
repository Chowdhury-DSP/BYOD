#pragma once

#include <pch.h>

struct LabelWithCentredEditor : Label
{
    TextEditor* createEditorComponent() override
    {
        auto* editor = Label::createEditorComponent();
        editor->setJustification (Justification::centred);
        editor->setMultiLine (false);

        return editor;
    }
};
