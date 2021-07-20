#include "BoardViewport.h"

class BoardViewport::ScrollLNF : public ByodLNF
{
public:
    ScrollLNF (const OwnedArray<ProcessorEditor>& processorEditors) : editors (processorEditors)
    {
    }

    void drawScrollbar (Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override
    {
        ignoreUnused (x, isMouseOver, isMouseDown, isScrollbarVertical);

        // get colour and name for each editor
        Array<std::pair<Colour, String>> editorBarDetails;
        editorBarDetails.ensureStorageAllocated (editors.size());
        for (const auto* editor : editors)
            editorBarDetails.add (std::make_pair (editor->getUIOptions().backgroundColour, editor->getProcPtr()->getName()));

        // paint colours
        constexpr int fadeWidth = 35;
        auto edWidth = width / editors.size();
        for (int i = 0; i < editors.size(); ++i)
        {
            if (i + 1 == editors.size())
            {
                auto rect = Rectangle { edWidth * i + fadeWidth / 2, y, edWidth - fadeWidth / 2, height };
                g.setColour (editorBarDetails[i].first);
                g.fillRect (rect);

                continue;
            }

            if (i == 0)
            {
                auto rect = Rectangle { edWidth * i, y, edWidth - fadeWidth / 2, height };
                g.setColour (editorBarDetails[i].first);
                g.fillRect (rect);
            }
            else
            {
                auto rect = Rectangle { edWidth * i + fadeWidth / 2, y, edWidth - fadeWidth, height };
                g.setColour (editorBarDetails[i].first);
                g.fillRect (rect);
            }

            auto gradRect = Rectangle { edWidth * (i + 1) - fadeWidth / 2, y, fadeWidth, height };
            g.setGradientFill (ColourGradient::horizontal (editorBarDetails[i].first, editorBarDetails[i + 1].first, gradRect.toFloat()));
            g.fillRect (gradRect);
        }

        // paint names
        for (int i = 0; i < editors.size(); ++i)
        {
            g.setColour (editorBarDetails[i].first.contrasting());
            g.setFont (18.0f);

            auto rect = Rectangle { edWidth * i, y, edWidth, height };
            g.drawFittedText (editorBarDetails[i].second, rect, Justification::centred, 1);
        }

        // paint "thumb"
        Rectangle<int> thumbBounds { thumbStartPosition, y, thumbSize, height };
        g.setColour (scrollbar.findColour (ScrollBar::ColourIds::thumbColourId));
        g.drawRoundedRectangle (thumbBounds.reduced (1).toFloat(), 4.0f, 5.0f);
    }

private:
    const OwnedArray<ProcessorEditor>& editors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrollLNF)
};

BoardViewport::BoardViewport (ProcessorChain& procChain) : comp (procChain)
{
    setViewedComponent (&comp, false);
    scrollLNF = std::make_unique<ScrollLNF> (comp.getEditors());

    getHorizontalScrollBar().setColour (ScrollBar::thumbColourId, Colours::black.brighter (0.1f));
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
    setScrollBarThickness (30);
}
