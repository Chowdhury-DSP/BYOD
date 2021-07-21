#include "BoardViewport.h"

class BoardViewport::ScrollLNF : public ByodLNF
{
public:
    ScrollLNF (const OwnedArray<ProcessorEditor>& processorEditors) : editors (processorEditors)
    {
    }

    void drawScrollbar (Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override
    {
        ignoreUnused (isMouseOver, isMouseDown, isScrollbarVertical);

        g.setColour (Colours::black.withAlpha (0.4f));
        Rectangle<int> backBounds { x, y, width, height };
        g.fillRect (backBounds);

        // show editor snapshots
        constexpr float thumbThick = 3.0f;
        const auto edWidth = (float) editors[0]->getWidth();
        const auto edHeight = (float) editors[0]->getHeight();
        const auto nEditors = editors.size();

        const auto scaleFactor = float (height - 2.0f * thumbThick) / edHeight;
        const auto snapWidth = int (edWidth * scaleFactor);
        const auto snapXInc = snapWidth * nEditors < width ? snapWidth : int (float (width - snapWidth) / float (nEditors - 1));

        g.setOpacity (1.0f);
        int snapX = x;
        for (int i = 0; i < nEditors; ++i)
        {
            auto snapshot = editors[i]->createComponentSnapshot (editors[i]->getLocalBounds(), true, scaleFactor);

            g.drawImageAt (snapshot, snapX, y + (int) thumbThick);
            snapX += i < nEditors - 1 ? snapXInc : snapWidth;
        }

        // paint "thumb"
        const auto adjustedThumbX = int (((float) thumbStartPosition / (float) width) * (float) snapX);
        const auto adjustedThumbWidth = int (((float) thumbSize / (float) width) * (float) snapX);

        Rectangle<int> thumbBounds { adjustedThumbX, y, adjustedThumbWidth, height };
        g.setColour (scrollbar.findColour (ScrollBar::ColourIds::thumbColourId));
        g.drawRoundedRectangle (thumbBounds.reduced (1).toFloat(), thumbThick, thumbThick);
    }

private:
    const OwnedArray<ProcessorEditor>& editors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrollLNF)
};

BoardViewport::BoardViewport (ProcessorChain& procChain) : comp (procChain)
{
    setViewedComponent (&comp, false);
    scrollLNF = std::make_unique<ScrollLNF> (comp.getEditors());

    getHorizontalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0xFF0EDED4));
    getHorizontalScrollBar().setLookAndFeel (scrollLNF.get());
    setScrollBarsShown (false, true);
}

BoardViewport::~BoardViewport()
{
    getHorizontalScrollBar().setLookAndFeel (nullptr);
}

void BoardViewport::resized()
{
    comp.setBounds (0, 0, comp.getIdealWidth (getWidth()), getHeight());
    setScrollBarThickness (BoardComponent::yOffset * 2);
}

void BoardViewport::scrollBarMoved (ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    Viewport::scrollBarMoved (scrollBarThatHasMoved, newRangeStart);
    getHorizontalScrollBar().repaint();
}
