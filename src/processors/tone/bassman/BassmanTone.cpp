#include "BassmanTone.h"
#include "../../ParameterHelpers.h"

BassmanTone::BassmanTone (UndoManager* um) : BaseProcessor ("Bassman Tone", createParameterLayout(), um)
{
    bassParam = vts.getRawParameterValue ("bass");
    midParam = vts.getRawParameterValue ("mid");
    trebleParam = vts.getRawParameterValue ("treble");

    uiOptions.backgroundColour = Colours::forestgreen;
    uiOptions.powerColour = Colours::yellow.brighter();
    uiOptions.info.description = "Virtual analog emulation of the Fender Bassman tone stack.";
    uiOptions.info.authors = StringArray { "Samuel Schachter", "Jatin Chowdhury" };
    uiOptions.info.infoLink = URL { "https://github.com/schachtersam32/WaveDigitalFilters_Sharc" };
}

AudioProcessorValueTreeState::ParameterLayout BassmanTone::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "bass", "Bass", 1.0f);
    createPercentParameter (params, "mid", "Tilt", 0.0f);
    createPercentParameter (params, "treble", "Treble", 0.0f);

    return { params.begin(), params.end() };
}

std::tuple<double, double, double> BassmanTone::cookParameters() const
{
    auto lowPot = 0.999f * std::pow (*bassParam, 0.25f) + 0.001f;
    auto midPot = 0.999f * (*midParam) + 0.001f;
    auto highPot = 0.99999f * std::pow (*trebleParam, 0.25f) + 0.00001f;

    return std::make_tuple ((double) lowPot, (double) midPot, (double) highPot);
}

void BassmanTone::prepare (double sampleRate, int samplesPerBlock)
{
    auto [lowPot, midPot, highPot] = cookParameters();
    for (int ch = 0; ch < 2; ++ch)
    {
        wdf[ch] = std::make_unique<BassmanToneStack> (sampleRate);
        wdf[ch]->setParams (highPot, 1.0 - lowPot, 1.0 - midPot, true);
    }

    dBuffer.setSize (2, samplesPerBlock);
    dBuffer.clear();
}

void BassmanTone::processAudio (AudioBuffer<float>& buffer)
{
    dBuffer.makeCopyOf (buffer, true);
    auto [lowPot, midPot, highPot] = cookParameters();
    for (int ch = 0; ch < dBuffer.getNumChannels(); ++ch)
    {
        wdf[ch]->setParams (highPot, 1.0 - lowPot, 1.0 - midPot);
        auto* x = dBuffer.getWritePointer (ch);
        wdf[ch]->process (x, dBuffer.getNumSamples());
    }

    buffer.makeCopyOf (dBuffer, true);
}
