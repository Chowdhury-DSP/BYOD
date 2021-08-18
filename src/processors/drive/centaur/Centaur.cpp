#include "Centaur.h"

Centaur::Centaur (UndoManager* um) : BaseProcessor ("Centaur", createParameterLayout(), um)
{
    levelParam = vts.getRawParameterValue ("level");

    uiOptions.backgroundColour = Colour (0xFFDAA520);
    uiOptions.powerColour = Colour (0xFF14CBF2).brighter (0.5f);
    uiOptions.info.description = "Emulation of the Klon Centaur overdrive pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    uiOptions.info.infoLink = URL { "https://github.com/jatinchowdhury18/KlonCentaur" };
}

AudioProcessorValueTreeState::ParameterLayout Centaur::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "gain", "Gain", 0.5f);
    createPercentParameter (params, "level", "Level", 0.5f);

    return { params.begin(), params.end() };
}

void Centaur::prepare (double sampleRate, int samplesPerBlock)
{
    gainStageProc = std::make_unique<GainStageProc> (vts, sampleRate);
    gainStageProc->reset (sampleRate, samplesPerBlock);

    for (int ch = 0; ch < 2; ++ch)
    {
        inProc[ch].prepare ((float) sampleRate);
        outProc[ch].prepare ((float) sampleRate);
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

void Centaur::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        // Input buffer
        FloatVectorOperations::multiply (x, 0.5f, numSamples);
        inProc[ch].processBlock (x, numSamples);
        FloatVectorOperations::clip (x, x, -4.5f, 4.5f, numSamples); // op amp clipping
    }

    gainStageProc->processBlock (buffer);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        
        outProc[ch].setLevel (*levelParam);
        outProc[ch].processBlock (x, numSamples);
    }

    dcBlocker.processAudio (buffer);
}

