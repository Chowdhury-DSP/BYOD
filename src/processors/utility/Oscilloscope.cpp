#include "Oscilloscope.h"
#include "../ParameterHelpers.h"

namespace
{
constexpr int scopeFps = 30;
}

Oscilloscope::Oscilloscope (UndoManager* um) : BaseProcessor ("Oscilloscope", createParameterLayout(), um, 1, 0)
{
    uiOptions.backgroundColour = Colours::silver.brighter (0.2f);
    uiOptions.powerColour = Colours::red;
    uiOptions.info.description = "Basic Oscilloscope.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Oscilloscope::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    return { params.begin(), params.end() };
}

void Oscilloscope::prepare (double sampleRate, int samplesPerBlock)
{
    scopeTask.prepare (sampleRate, samplesPerBlock, 1);
}

void Oscilloscope::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels > 1) // collapse to mono...
    {
        buffer.addFrom (0, 0, buffer, 1, 0, numSamples);
        buffer.applyGain (0, 0, numSamples, 1.0f / MathConstants<float>::sqrt2);
    }

    scopeTask.pushSamples (0, buffer.getReadPointer (0), numSamples);
}

void Oscilloscope::inputConnectionChanged (int /*portIndex*/, bool /*wasConnected*/)
{
    scopeTask.reset();
}

//===================================================================
void Oscilloscope::ScopeBackgroundTask::prepareTask (double sampleRate, int /*samplesPerBlock*/, int& requstedBlockSize, int& waitMs)
{
    constexpr auto millisecondsToDisplay = 20.0;
    samplesToDisplay = int (millisecondsToDisplay * 0.001 * sampleRate) - 1;
    triggerBuffer = int (sampleRate / 50.0);

    requstedBlockSize = samplesToDisplay + triggerBuffer;

    waitMs = int (1000.0 / (double) scopeFps);
}

void Oscilloscope::ScopeBackgroundTask::resetTask()
{
    ScopedLock sl (crit);
    scopePath.clear();
    scopePath.startNewSubPath (mapXY (0, 0.0f));
    scopePath.lineTo (mapXY (samplesToDisplay, 0.0f));
}

Point<float> Oscilloscope::ScopeBackgroundTask::mapXY (int sampleIndex, float yVal) const
{
    return Point { jmap (float (sampleIndex), 0.0f, float (samplesToDisplay), bounds.getX(), bounds.getRight()),
                   jmap (yVal, -1.0f, 1.0f, bounds.getBottom(), bounds.getY()) };
}

void Oscilloscope::ScopeBackgroundTask::runTask (const AudioBuffer<float>& buffer)
{
    const auto* data = buffer.getReadPointer (0);

    // trigger from last zero-crossing
    int triggerOffset = triggerBuffer - 1;
    auto sign = data[triggerOffset] > 0.0f;

    while (! sign && triggerOffset > 0)
        sign = data[triggerOffset--] > 0.0f;

    while (sign && triggerOffset > 0)
        sign = data[triggerOffset--] > 0.0f;

    // update path
    ScopedLock sl (crit);
    if (bounds == Rectangle<float> {})
        return;

    scopePath.clear();
    scopePath.startNewSubPath (mapXY (0, data[triggerOffset]));
    for (int i = 1; i < samplesToDisplay; ++i)
        scopePath.lineTo (mapXY (i, data[triggerOffset + i]));
}

void Oscilloscope::ScopeBackgroundTask::setBounds (Rectangle<int> newBounds)
{
    ScopedLock sl (crit);
    bounds = newBounds.toFloat();
}

Path Oscilloscope::ScopeBackgroundTask::getScopePath() const noexcept
{
    ScopedLock sl (crit);
    return scopePath;
}

//===================================================================
bool Oscilloscope::getCustomComponents (OwnedArray<Component>& customComps)
{
    struct ScopeComp : public Component,
                       private Timer
    {
        explicit ScopeComp (ScopeBackgroundTask& sTask) : scopeTask (sTask)
        {
            scopeTask.setShouldBeRunning (true);
            startTimerHz (scopeFps);
        }

        ~ScopeComp() override
        {
            scopeTask.setShouldBeRunning (false);
        }

        void enablementChanged() override
        {
            scopeTask.setShouldBeRunning (isEnabled());
        }

        void paint (Graphics& g) override
        {
            auto b = getLocalBounds();

            g.setColour (Colours::black);
            g.fillRoundedRectangle (b.toFloat(), 15.0f);

            constexpr float lineThickness = 2.0f;
            g.setColour (Colours::red.withAlpha (isEnabled() ? 1.0f : 0.6f));
            auto scopePath = scopeTask.getScopePath();
            g.strokePath (scopePath, juce::PathStrokeType (lineThickness));
        }

        void resized() override { scopeTask.setBounds (getLocalBounds()); }
        void timerCallback() override { repaint(); }

        ScopeBackgroundTask& scopeTask;
    };

    customComps.add (std::make_unique<ScopeComp> (scopeTask));
    return false;
}
