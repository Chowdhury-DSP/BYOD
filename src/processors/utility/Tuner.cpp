#include "Tuner.h"
#include "../ParameterHelpers.h"

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
void Tuner::TunerBackgroundTask::prepare (double sampleRate, int samplesPerBlock)
{
    if (isThreadRunning())
        stopThread (-1);

    isPrepared = false;

    constexpr double lowestFreqHz = 34.0; // lowest string on a bass guitar tuned to low D is 36.71 Hz
    autocorrelationSize = int (sampleRate / lowestFreqHz) + 1;
    data.resize (2 * jmax (autocorrelationSize, samplesPerBlock));

    auto refreshTime = (double) data.size() / sampleRate; // time (seconds) for the whole buffer to be refreshed
    waitMilliseconds = int (1000.0 * refreshTime);

    fs = sampleRate;
    writePosition = 0;
    curFreqHz = 0.0;
    isPrepared = true;

    if (shouldBeRunning)
        startThread();
}

void Tuner::TunerBackgroundTask::pushSamples (const float* samples, int numSamples)
{
    data.push (samples, numSamples);
    writePosition = data.getWritePointer();
}

void Tuner::TunerBackgroundTask::run()
{
    while (1)
    {
        if (threadShouldExit())
            return;

        computeCurrentFrequency();
        wait (waitMilliseconds);
    }
}

void Tuner::TunerBackgroundTask::computeCurrentFrequency()
{
    const auto* latestData = data.data (writePosition - autocorrelationSize);

    // fail early if buffer is silent!
    auto minMax = FloatVectorOperations::findMinAndMax (latestData, autocorrelationSize);
    if (std::abs (minMax.getStart()) < 1.0e-2f && std::abs (minMax.getEnd()) < 1.0e-2f)
    {
        curFreqHz = 0.0f;
        return;
    }

    // simple autocorrelation-based frequency detection
    float sumOld = 0.0;
    float sum = 0.0;
    int peakDetectorState = 1;
    int period = 0;
    float thresh = 0.0f;

    for (int i = 0; i < autocorrelationSize; ++i)
    {
        sumOld = sum;
        sum = std::inner_product (latestData, &latestData[autocorrelationSize - i], &latestData[i], 0.0f);

        // Peak Detect State Machine
        if (peakDetectorState == 2 && (sum - sumOld) <= 0)
        {
            period = i;
            break;
        }
        if (peakDetectorState == 1 && (sum > thresh) && (sum - sumOld) > 0)
            peakDetectorState = 2;

        if (i == 0)
        {
            thresh = sum * 0.5f;
            peakDetectorState = 1;
        }
    }

    if (period > 0)
        curFreqHz = fs / (double) period;
}

void Tuner::TunerBackgroundTask::setShouldBeRunning (bool shouldRun)
{
    shouldBeRunning = shouldRun;

    if (! shouldRun && isThreadRunning())
    {
        stopThread (-1);
        return;
    }

    if (isPrepared && shouldRun && ! isThreadRunning())
    {
        startThread();
        return;
    }
}

//===================================================================
void Tuner::getCustomComponents (OwnedArray<Component>& customComps)
{
    struct TunerComp : public Component,
                       private Timer
    {
        TunerComp (TunerBackgroundTask& tTask) : tunerTask (tTask)
        {
            tunerTask.setShouldBeRunning (true);
            startTimerHz (20);
        }

        ~TunerComp() override
        {
            tunerTask.setShouldBeRunning (false);
        }

        void paint (Graphics& g) override
        {
            g.setColour (Colours::black);
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 25.0f);

            g.setColour (Colours::yellow);
            g.setFont (18.0f);
            auto freqString = String (tunerTask.getCurrentFreqHz(), 2) + " Hz";
            g.drawFittedText (freqString, getLocalBounds(), Justification::centred, 1);
        }

        void timerCallback() override
        {
            repaint();
        }

        TunerBackgroundTask& tunerTask;
    };

    customComps.add (std::make_unique<TunerComp> (tunerTask));
}
