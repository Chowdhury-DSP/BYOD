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
    hysteresisProc.reset();
    hysteresisProc.setSampleRate (sampleRate);

    doubleBuffer.setSize (2, samplesPerBlock);
}

void Hysteresis::processAudio (AudioBuffer<float>& buffer)
{
    buffer.applyGain (2.0f);

    doubleBuffer.makeCopyOf (buffer, true);

    auto* leftPtr = doubleBuffer.getWritePointer (0);
    auto* rightPtr = doubleBuffer.getNumChannels() > 1 ? doubleBuffer.getWritePointer (1) : leftPtr;

    hysteresisProc.setParameters (driveParam->load(), widthParam->load(), satParam->load());
    hysteresisProc.processBlock (leftPtr, rightPtr, buffer.getNumSamples());

    buffer.makeCopyOf (doubleBuffer, true);
}
