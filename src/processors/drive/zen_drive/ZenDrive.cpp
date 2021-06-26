#include "ZenDrive.h"
#include "../../ParameterHelpers.h"

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
    for (int ch = 0; ch < 2; ++ch)
        wdf[ch] = std::make_unique<ZenDriveWDF> ((float) sampleRate);

    dcBlocker.prepare (sampleRate, samplesPerBlock);
}

void ZenDrive::processAudio (AudioBuffer<float>& buffer)
{
    buffer.applyGain (0.5f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch]->setParameters (1.0f - *voiceParam, ParameterHelpers::logPot (*gainParam));
        auto* x = buffer.getWritePointer (ch);
        wdf[ch]->process (x, buffer.getNumSamples());
    }

    dcBlocker.processAudio (buffer);
}
