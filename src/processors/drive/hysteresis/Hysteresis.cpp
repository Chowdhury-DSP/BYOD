#include "Hysteresis.h"
#include "../../ParameterHelpers.h"

Hysteresis::Hysteresis (UndoManager* um) : BaseProcessor ("Hysteresis", createParameterLayout(), um)
{
    satParam = vts.getRawParameterValue ("sat");
    driveParam = vts.getRawParameterValue ("drive");
    widthParam = vts.getRawParameterValue ("width");

    uiOptions.backgroundColour = Colour (0xFF8B3232);
    uiOptions.powerColour = Colour (0xFFEAA92C);
    uiOptions.info.description = "Nonlinear hysteresis distortion, similar to the distortion created by magnetic tape, or an overdriven transformer.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout Hysteresis::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "sat", "Saturation", 0.5f);
    createPercentParameter (params, "drive", "Drive", 0.5f);
    createPercentParameter (params, "width", "Width", 0.5f);

    return { params.begin(), params.end() };
}

void Hysteresis::prepare (double sampleRate, int samplesPerBlock)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        hysteresisProc[ch].reset();
        hysteresisProc[ch].setSampleRate (sampleRate);
    }

    doubleBuffer.setSize (2, samplesPerBlock);
}

void Hysteresis::processAudio (AudioBuffer<float>& buffer)
{
    buffer.applyGain (2.0f);

    doubleBuffer.makeCopyOf (buffer, true);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = doubleBuffer.getWritePointer (ch);

        hysteresisProc[ch].setParameters (driveParam->load(), widthParam->load(), satParam->load());
        hysteresisProc[ch].processBlock (x, buffer.getNumSamples());
    }

    buffer.makeCopyOf (doubleBuffer);
}
