#include "BassmanTone.h"
#include "../../ParameterHelpers.h"

BassmanTone::BassmanTone (UndoManager* um) : BaseProcessor ("Bassman Tone", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (bassParam, vts, "bass");
    loadParameterPointer (midParam, vts, "mid");
    loadParameterPointer (trebleParam, vts, "treble");

    uiOptions.backgroundColour = Colours::forestgreen;
    uiOptions.powerColour = Colours::yellow.brighter();
    uiOptions.info.description = "Virtual analog emulation of the Fender Bassman tone stack.";
    uiOptions.info.authors = StringArray { "Samuel Schachter", "Jatin Chowdhury" };
    uiOptions.info.infoLink = "https://github.com/schachtersam32/WaveDigitalFilters_Sharc";
}

ParamLayout BassmanTone::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "bass", "Bass", 1.0f);
    createPercentParameter (params, "mid", "Tilt", 0.0f);
    createPercentParameter (params, "treble", "Treble", 0.0f);

    return { params.begin(), params.end() };
}

auto BassmanTone::cookParameters() const
{
    auto lowPot = 0.999f * std::pow (*bassParam, 0.25f) + 0.001f;
    auto midPot = 0.999f * (*midParam) + 0.001f;
    auto highPot = 0.99999f * std::pow (*trebleParam, 0.25f) + 0.00001f;

    return std::make_tuple (lowPot, midPot, highPot);
}

void BassmanTone::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    auto [lowPot, midPot, highPot] = cookParameters();
    for (auto& ch : wdf)
    {
        ch.prepare (sampleRate);
        ch.setParams (highPot, 1.0f - lowPot, 1.0f - midPot, true);
    }
}

void BassmanTone::processAudio (AudioBuffer<float>& buffer)
{
    auto [lowPot, midPot, highPot] = cookParameters();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParams (highPot, 1.0f - lowPot, 1.0f - midPot);
        auto* x = buffer.getWritePointer (ch);
        wdf[ch].process (x, buffer.getNumSamples());
    }
}
