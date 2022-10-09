#include "TextSlider.h"

TextSlider::TextSlider (const chowdsp::FloatParameter* param, const HostContextProvider& hcp)
    : parameter (param),
      hostContextProvider (hcp)
{
    setLookAndFeel (lnfAllocator->getLookAndFeel<BottomBarLNF>());
    setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    setMouseDragSensitivity (500);
}

TextSlider::~TextSlider()
{
    setLookAndFeel (nullptr);
}

void TextSlider::resized()
{
    Slider::resized();
    setTextBoxStyle (Slider::TextBoxLeft, false, 0, getHeight());
}

void TextSlider::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        hostContextProvider.showParameterContextPopupMenu (parameter);
        return;
    }
    Slider::mouseDown (e);
}

void TextSlider::mouseUp (const MouseEvent& e)
{
    Slider::mouseUp (e);

    multiClicking = e.getNumberOfClicks() > 1;
    bool dontShowLabel = e.mouseWasDraggedSinceMouseDown() || e.mods.isAnyModifierKeyDown() || e.mods.isPopupMenu() || multiClicking;
    if (! dontShowLabel)
    {
        Timer::callAfterDelay (270,
                               [safeComp = Component::SafePointer (this)]
                               {
                                   if (safeComp == nullptr)
                                       return; // this component was deleted while waiting for the timer!

                                   if (safeComp->multiClicking)
                                   {
                                       safeComp->multiClicking = false;
                                       return;
                                   }

                                   safeComp->showTextBox();
                               });
    }
}
