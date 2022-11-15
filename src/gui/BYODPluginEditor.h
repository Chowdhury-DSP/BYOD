#pragma once

#include "BYOD.h"
#include "TitleBar.h"
#include "pedalboard/BoardViewport.h"
#include "toolbar/ToolBar.h"
#include "utils/ErrorMessageView.h"

class BYODPluginEditor : public AudioProcessorEditor
{
public:
    explicit BYODPluginEditor (BYOD& plugin);
    ~BYODPluginEditor() override;

    void paint (Graphics& g) override;
    void resized() override;

    int getControlParameterIndex (Component& c) override;

    auto& getErrorMessageView() { return errorMessageView; }

private:
    void setResizeBehaviour();

    BYOD& plugin;
    HostContextProvider hostContextProvider;

    ComponentBoundsConstrainer constrainer;

    chowdsp::SharedLNFAllocator lnfAllocator;
    TitleBar titleBar;
    BoardViewport board;
    ToolBar toolBar;
    ErrorMessageView errorMessageView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BYODPluginEditor)
};
