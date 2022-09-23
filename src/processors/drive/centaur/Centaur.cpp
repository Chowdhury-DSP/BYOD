#include "Centaur.h"

namespace
{
const String levelTag = "level";
const String modeTag = "mode";
} // namespace

Centaur::Centaur (UndoManager* um) : BaseProcessor ("Centaur", createParameterLayout(), um),
                                     gainStageML (vts)
{
    chowdsp::ParamUtils::loadParameterPointer (levelParam, vts, levelTag);
    modeParam = vts.getRawParameterValue (modeTag);
    addPopupMenuParameter (modeTag);

    uiOptions.backgroundColour = Colour (0xFFDAA520);
    uiOptions.powerColour = Colour (0xFF14CBF2).brighter (0.5f);
    uiOptions.info.description = "Emulation of the Klon Centaur overdrive pedal. Use the right-click menu to enable neural mode.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    uiOptions.info.infoLink = "https://github.com/jatinchowdhury18/KlonCentaur";
}

ParamLayout Centaur::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "gain", "Gain", 0.5f);
    createPercentParameter (params, levelTag, "Level", 0.5f);
    emplace_param<AudioParameterChoice> (params, modeTag, "Mode", StringArray { "Traditional", "Neural" }, 0);

    return { params.begin(), params.end() };
}

void Centaur::prepare (double sampleRate, int samplesPerBlock)
{
    gainStageProc = std::make_unique<GainStageProc> (vts, sampleRate);
    gainStageProc->reset (sampleRate, samplesPerBlock);
    gainStageML.reset (sampleRate, samplesPerBlock);

    for (int ch = 0; ch < 2; ++ch)
    {
        inProc[ch].prepare ((float) sampleRate);
        outProc[ch].prepare ((float) sampleRate);
    }

    fadeBuffer.setSize (2, samplesPerBlock);

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

    const bool useML = *modeParam == 1.0f;
    if (useML == useMLPrev)
    {
        if (useML) // use rnn
            gainStageML.processBlock (buffer);
        else // use circuit model
            gainStageProc->processBlock (buffer);
    }
    else
    {
        fadeBuffer.makeCopyOf (buffer, true);

        if (useML) // use rnn
        {
            gainStageML.processBlock (buffer);
            gainStageProc->processBlock (fadeBuffer);
        }
        else // use circuit model
        {
            gainStageProc->processBlock (buffer);
            gainStageML.processBlock (fadeBuffer);
        }

        buffer.applyGainRamp (0, numSamples, 0.0f, 1.0f);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFromWithRamp (ch, 0, fadeBuffer.getReadPointer (ch), numSamples, 1.0f, 0.0f);

        useMLPrev = useML;
    }

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        outProc[ch].setLevel (*levelParam);
        outProc[ch].processBlock (x, numSamples);
    }

    dcBlocker.processAudio (buffer);
}
