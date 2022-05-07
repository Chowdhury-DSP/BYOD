#pragma once

#include <pch.h>

class BYOD;
class TitleBar : public Component
{
public:
    explicit TitleBar (BYOD& plugin);
    ~TitleBar() override;

    void paint (Graphics& g) override;
    void resized() override;

private:
    chowdsp::TitleComp titleComp;

    struct BYODInfoComp;
    std::unique_ptr<BYODInfoComp> infoComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TitleBar)
};
