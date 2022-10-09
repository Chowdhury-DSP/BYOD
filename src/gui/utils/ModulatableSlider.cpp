#include "ModulatableSlider.h"

ModulatableSlider::ModulatableSlider (const chowdsp::FloatParameter& p, const HostContextProvider& hcp) : param (p),
                                                                                                          hostContextProvider (hcp)
{
    if (hostContextProvider.supportsParameterModulation)
        startTimerHz (25);
}

void ModulatableSlider::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float modSliderPos)
{
    int diameter = (width > height) ? height : width;
    if (diameter < 16)
        return;

    juce::Point<float> centre ((float) x + std::floor ((float) width * 0.5f + 0.5f), (float) y + std::floor ((float) height * 0.5f + 0.5f));
    diameter -= (diameter % 2 == 1) ? 9 : 8;
    float radius = (float) diameter * 0.5f;
    x = int (centre.x - radius);
    y = int (centre.y - radius);

    const auto bounds = juce::Rectangle<int> (x, y, diameter, diameter).toFloat();

    auto b = sharedAssets->pointer->getBounds().toFloat();
    sharedAssets->pointer->setTransform (AffineTransform::rotation (MathConstants<float>::twoPi * ((sliderPos - 0.5f) * 300.0f / 360.0f),
                                                                    b.getCentreX(),
                                                                    b.getCentreY()));

    const auto alpha = isEnabled() ? 1.0f : 0.4f;
    auto knobBounds = (bounds * 0.75f).withCentre (centre);
    sharedAssets->knob->drawWithin (g, knobBounds, RectanglePlacement::stretchToFit, alpha);
    sharedAssets->pointer->drawWithin (g, knobBounds, RectanglePlacement::stretchToFit, alpha);

    static constexpr auto rotaryStartAngle = MathConstants<float>::pi * 1.2f;
    static constexpr auto rotaryEndAngle = MathConstants<float>::pi * 2.8f;
    const auto toAngle = rotaryStartAngle + modSliderPos * (rotaryEndAngle - rotaryStartAngle);
    constexpr float arcFactor = 0.9f;

    juce::Path valueArc;
    valueArc.addPieSegment (bounds, rotaryStartAngle, rotaryEndAngle, arcFactor);
    g.setColour (findColour (juce::Slider::trackColourId).withMultipliedAlpha (alpha));
    g.fillPath (valueArc);
    valueArc.clear();

    valueArc.addPieSegment (bounds, rotaryStartAngle, toAngle, arcFactor);
    g.setColour (findColour (juce::Slider::thumbColourId).withMultipliedAlpha (alpha));
    g.fillPath (valueArc);
}

void ModulatableSlider::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float modSliderPos)
{
    const auto horizontal = isHorizontal();
    auto trackWidth = juce::jmin (6.0f, horizontal ? (float) height * 0.25f : (float) width * 0.25f);

    juce::Point startPoint (horizontal ? (float) x : (float) x + (float) width * 0.5f,
                            horizontal ? (float) y + (float) height * 0.5f : (float) (height + y));

    juce::Point endPoint (horizontal ? (float) (width + x) : startPoint.x,
                          horizontal ? startPoint.y : (float) y);

    juce::Path backgroundTrack;
    backgroundTrack.startNewSubPath (startPoint);
    backgroundTrack.lineTo (endPoint);

    const auto alphaMult = isEnabled() ? 1.0f : 0.4f;
    g.setColour (findColour (juce::Slider::backgroundColourId).withAlpha (alphaMult));
    g.strokePath (backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

    juce::Path valueTrack;
    const auto maxPoint = [&]
    {
        auto kx = horizontal ? sliderPos : ((float) x + (float) width * 0.5f);
        auto ky = horizontal ? ((float) y + (float) height * 0.5f) : sliderPos;
        return juce::Point { kx, ky };
    }();
    const auto modPoint = [&]
    {
        auto kmx = horizontal ? modSliderPos : ((float) x + (float) width * 0.5f);
        auto kmy = horizontal ? ((float) y + (float) height * 0.5f) : modSliderPos;
        return juce::Point { kmx, kmy };
    }();

    valueTrack.startNewSubPath (startPoint);
    valueTrack.lineTo (modPoint);
    g.setColour (findColour (juce::Slider::thumbColourId).withAlpha (alphaMult));
    g.strokePath (valueTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

    auto thumbWidth = getLookAndFeel().getSliderThumbRadius (*this);
    auto thumbRect = juce::Rectangle<float> (static_cast<float> (thumbWidth),
                                             static_cast<float> (thumbWidth))
                         .withCentre (maxPoint);
    sharedAssets->knob->drawWithin (g, thumbRect, juce::RectanglePlacement::stretchToFit, alphaMult);
}

void ModulatableSlider::paint (Graphics& g)
{
    auto& lf = getLookAndFeel();
    auto layout = lf.getSliderLayout (*this);
    const auto sliderRect = layout.sliderBounds;

    modulatedValue = param.getCurrentValue();
    if (isRotary())
    {
        const auto sliderPos = (float) valueToProportionOfLength (getValue());
        const auto modSliderPos = (float) jlimit (0.0, 1.0, valueToProportionOfLength (modulatedValue));
        drawRotarySlider (g,
                          sliderRect.getX(),
                          sliderRect.getY(),
                          sliderRect.getWidth(),
                          sliderRect.getHeight(),
                          sliderPos,
                          modSliderPos);
    }
    else
    {
        const auto normRange = NormalisableRange { getRange() };
        const auto sliderPos = getPositionOfValue (getValue());
        const auto modSliderPos = getPositionOfValue (normRange.convertTo0to1 (modulatedValue));
        drawLinearSlider (g,
                          sliderRect.getX(),
                          sliderRect.getY(),
                          sliderRect.getWidth(),
                          sliderRect.getHeight(),
                          sliderPos,
                          modSliderPos);
    }
}

void ModulatableSlider::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        hostContextProvider.showParameterContextPopupMenu (&param);
        return;
    }

    Slider::mouseDown (e);
}

void ModulatableSlider::timerCallback()
{
    const auto newModulatedValue = param.getCurrentValue();
    if (std::abs (modulatedValue - newModulatedValue) < 0.01)
        return;

    repaint();
}
