#include "Tuner.h"
#include "../ParameterHelpers.h"

namespace
{
constexpr int tunerRefreshHz = 24;
}

Tuner::Tuner (UndoManager* um) : BaseProcessor ("Tuner", createParameterLayout(), um, 1, 0)
{
    uiOptions.backgroundColour = Colours::silver.brighter (0.2f);
    uiOptions.powerColour = Colours::red;
    uiOptions.info.description = "A440 tuner.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout Tuner::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    return { params.begin(), params.end() };
}

void Tuner::prepare (double sampleRate, int samplesPerBlock)
{
    tunerTask.prepare (sampleRate, samplesPerBlock);
}

void Tuner::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels > 1) // collapse to mono...
    {
        buffer.addFrom (0, 0, buffer, 1, 0, numSamples);
        buffer.applyGain (0, 0, numSamples, 1.0f / MathConstants<float>::sqrt2);
    }

    tunerTask.pushSamples (buffer.getReadPointer (0), numSamples);
}

//===================================================================
void Tuner::TunerBackgroundTask::prepareTask (double sampleRate, int /*samplesPerBlock*/, int& requestedBlockSize)
{
    tuner.prepare (sampleRate);
    requestedBlockSize = tuner.getAutocorrelationSize();

    freqValSmoother.reset ((double) tunerRefreshHz, 0.15);
    freqValSmoother.setCurrentAndTargetValue ((double) tuner.getCurrentFrequencyHz());
}

void Tuner::TunerBackgroundTask::runTask (const float* data)
{
    tuner.process (data);
    curFreqHz = (double) tuner.getCurrentFrequencyHz();
}

double Tuner::TunerBackgroundTask::getCurrentFreqHz() noexcept
{
    freqValSmoother.setTargetValue (curFreqHz.load());
    return freqValSmoother.getNextValue();
}

//===================================================================
void Tuner::getCustomComponents (OwnedArray<Component>& customComps)
{
    struct TunerComp : public Component,
                       private Timer
    {
        explicit TunerComp (TunerBackgroundTask& tTask) : tunerTask (tTask)
        {
            tunerTask.setShouldBeRunning (true);
            startTimerHz (tunerRefreshHz);
        }

        ~TunerComp() override
        {
            tunerTask.setShouldBeRunning (false);
        }

        static float getAngleForCents (int cents)
        {
            const auto centsNorm = (float) cents / 50.0f;
            return degreesToRadians (60.0f * centsNorm);
        }

        static void drawTunerVizBackground (Graphics& g, Rectangle<int> bounds)
        {
            g.setColour (Colours::grey.brighter());

            const auto height = (float) bounds.getHeight();
            auto tickLength = height * 0.15f;
            const auto bottomCentre = Point { bounds.getCentreX(), bounds.getBottom() }.toFloat();
            for (auto cents : { -50, -25, -10, 0, 10, 25, 50 })
            {
                auto angle = getAngleForCents (cents);
                auto line = Line<float>::fromStartAndAngle (bottomCentre, height, angle);
                line = line.withShortenedStart (height - tickLength);
                g.drawLine (line, 2.5f);
            }

            g.setColour (Colours::grey.withAlpha (0.65f));
            tickLength = height * 0.075f;
            for (auto cents : { -45, -40, -35, -30, -20, -15, -5, 5, 15, 20, 25, 30, 35, 40, 45 })
            {
                auto angle = getAngleForCents (cents);
                auto line = Line<float>::fromStartAndAngle (bottomCentre, height, angle);
                line = line.withShortenedStart (height - tickLength);
                g.drawLine (line, 1.5f);
            }
        }

        static void drawTunerLine (Graphics& g, Rectangle<int> bounds, int cents)
        {
            const auto height = (float) bounds.getHeight();
            const auto lineLength = height * 0.925f;
            const auto bottomCentre = Point { bounds.getCentreX(), bounds.getBottom() }.toFloat();

            auto angle = getAngleForCents (cents);
            auto line = Line<float>::fromStartAndAngle (bottomCentre, lineLength, angle);
            g.drawLine (line, 2.5f);
        }

        void paint (Graphics& g) override
        {
            auto b = getLocalBounds();

            g.setColour (Colours::black);
            g.fillRoundedRectangle (b.toFloat(), 20.0f);

            auto nameHeight = proportionOfHeight (0.3f);
            auto nameBounds = b.removeFromTop (nameHeight);
            auto tunerVizBounds = b.reduced (proportionOfHeight (0.05f), 0);
            tunerVizBounds.removeFromTop (proportionOfHeight (0.025f));

            drawTunerVizBackground (g, tunerVizBounds);

            auto curFreqHz = tunerTask.getCurrentFreqHz();
            if (curFreqHz < 20.0)
                return;

            auto [noteNum, centsDouble] = chowdsp::TuningHelpers::frequencyHzToNoteAndCents (curFreqHz);
            const auto cents = (int) centsDouble;

            if (std::abs (cents) > 20)
                g.setColour (Colours::red.brighter());
            else if (std::abs (cents) > 5)
                g.setColour (Colours::yellow);
            else
                g.setColour (Colours::green.brighter());

            drawTunerLine (g, tunerVizBounds, cents);

            g.setFont ((float) nameHeight * 0.6f);
            auto freqString = String (curFreqHz, 1) + " Hz";
            auto noteString = MidiMessage::getMidiNoteName (noteNum, true, true, 4) + " (" + String ((int) cents) + ")";
            g.drawFittedText (noteString + " | " + freqString, nameBounds, Justification::centred, 1);
        }

        void timerCallback() override
        {
            repaint();
        }

        TunerBackgroundTask& tunerTask;
    };

    customComps.add (std::make_unique<TunerComp> (tunerTask));
}
