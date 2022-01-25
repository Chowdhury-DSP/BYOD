#include "ZenDrive.h"

ZenDrive::ZenDrive (UndoManager* um) : BaseProcessor ("Zen Drive", createParameterLayout(), um)
{
    voiceParam = vts.getRawParameterValue ("voice");
    gainParam = vts.getRawParameterValue ("gain");

    uiOptions.backgroundColour = Colours::cornsilk;
    uiOptions.info.description = "Virtual analog emulation of the ZenDrive overdrive pedal by Hermida Audio.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout ZenDrive::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "voice", "Voice", 0.5f);
    createPercentParameter (params, "gain", "Gain", 0.5f);

    return { params.begin(), params.end() };
}

void ZenDrive::prepare (double sampleRate, int samplesPerBlock)
{
    for (auto& wdfProc : wdf)
    {
        wdfProc.prepare (sampleRate);
        wdfProc.setParameters (1.0f - *voiceParam, ParameterHelpers::logPot (*gainParam));
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 10000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void ZenDrive::processAudio (AudioBuffer<float>& buffer)
{
    buffer.applyGain (0.5f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParameters (1.0f - *voiceParam, ParameterHelpers::logPot (*gainParam));
        wdf[ch].process (buffer.getWritePointer (ch), buffer.getNumSamples());
    }

    dcBlocker.processAudio (buffer);

    buffer.applyGain (Decibels::decibelsToGain (-10.0f));
}
