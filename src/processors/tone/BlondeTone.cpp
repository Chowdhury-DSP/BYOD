#include "BlondeTone.h"
#include "../ParameterHelpers.h"

namespace BlondeToneTags
{
const String bassTag = "bass";
const String midTag = "mid";
const String trebleTag = "treble";
} // namespace

BlondeTone::BlondeTone (UndoManager* um) : BaseProcessor ("Blonde Tone", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (bassParam, vts, BlondeToneTags::bassTag);
    loadParameterPointer (midsParam, vts, BlondeToneTags::midTag);
    loadParameterPointer (trebleParam, vts, BlondeToneTags::trebleTag);

    magic_enum::enum_for_each<EQBands> ([this] (auto val)
                                        { filters.setBandOnOff (val, true); });

    uiOptions.backgroundColour = Colour { 0xffdb8a2f };
    uiOptions.powerColour = Colour { 0xfff5d779 };
    uiOptions.info.description = "Tone stage based on the tone filters from the Joyo American Sound.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout BlondeTone::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createBipolarPercentParameter (params, BlondeToneTags::bassTag, "Bass", 0.0f);
    createBipolarPercentParameter (params, BlondeToneTags::midTag, "Mids", 0.0f);
    createBipolarPercentParameter (params, BlondeToneTags::trebleTag, "Treble", 0.0f);

    return { params.begin(), params.end() };
}

void BlondeTone::prepare (double sampleRate, int samplesPerBlock)
{
    filters.setFilterType (LevelFilter, 0);
    filters.setCutoffFrequency (LevelFilter, 2650.0f);
    filters.setQValue (LevelFilter, 0.5f);

    filters.setFilterType (MidsFilter, 1);
    filters.setQValue (MidsFilter, 0.3f);
    filters.setCutoffFrequency (MidsFilter, 685.0f);
    filters.setGainDB (MidsFilter, 0.0f);

    filters.setFilterType (BassFilter, 1);
    filters.setQValue (BassFilter, 0.45f);
    filters.setCutoffFrequency (BassFilter, 87.0f);
    filters.setGainDB (BassFilter, 0.0f);

    filters.setFilterType (TrebleFilter, 1);
    filters.setQValue (TrebleFilter, 0.45f);
    filters.setCutoffFrequency (TrebleFilter, 2800.0f);
    filters.setGainDB (TrebleFilter, 0.0f);

    filters.setFilterType (OutFilter, 2);
    filters.setCutoffFrequency (OutFilter, 8000.0f);

    filters.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 });
}

void BlondeTone::processAudio (AudioBuffer<float>& buffer)
{
    filters.setGainDB (MidsFilter, 12.0f * *midsParam);
    filters.setGainDB (BassFilter, 9.0f * *bassParam);
    filters.setGainDB (TrebleFilter, 10.0f * *trebleParam);

    filters.processBlock (buffer);
}
