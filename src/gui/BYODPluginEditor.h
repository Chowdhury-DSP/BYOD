#pragma once

#include "BYOD.h"
#include "TitleBar.h"
#include "pedalboard/BoardViewport.h"
#include "toolbar/ToolBar.h"

class BYODPluginEditor : public AudioProcessorEditor
{
public:
    explicit BYODPluginEditor (BYOD& plugin);
    ~BYODPluginEditor() override;

    void paint (Graphics& g) override;
    void resized() override;

private:
    void setResizeBehaviour();

    BYOD& plugin;
    const HostContextProvider hostContextProvider;

    ComponentBoundsConstrainer constrainer;

    chowdsp::SharedLNFAllocator lnfAllocator;
    TitleBar titleBar;
    BoardViewport board;
    ToolBar toolBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BYODPluginEditor)
};
