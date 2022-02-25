#pragma once

#include "../editors/ProcessorEditor.h"
#include "Cable.h"

class BoardComponent;
class CableViewConnectionHelper;
class CableViewPortLocationHelper;
class CableView : public Component,
                  private Timer
{
public:
    explicit CableView (const BoardComponent* comp);
    ~CableView() override;

    void paint (Graphics& g) override;

    auto* getConnectionHelper() { return connectionHelper.get(); }
    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingRemoved (const BaseProcessor* proc);

    void setScaleFactor (float newScaleFactor);

    using EditorPortPair = std::pair<ProcessorEditor*, int>;

private:
    void timerCallback() override;

    const BoardComponent* board = nullptr;
    OwnedArray<Cable> cables;

    float scaleFactor = 1.0f;

    friend class CableViewConnectionHelper;
    std::unique_ptr<CableViewConnectionHelper> connectionHelper;

    friend class CableViewPortLocationHelper;
    std::unique_ptr<CableViewPortLocationHelper> portLocationHelper;
    CableView::EditorPortPair outputPortToHighlight { nullptr, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableView)
};
