#include "PresetInfoHelpers.h"

namespace PresetInfoHelpers
{
static const Identifier isPublicTag { "is_public" };
static const Identifier presetIDTag { "preset_id" };

void setIsPublic (chowdsp::Preset& preset, bool shouldBePublic)
{
    preset.extraInfo.setAttribute (isPublicTag, shouldBePublic);
}

bool getIsPublic (const chowdsp::Preset& preset)
{
    return preset.extraInfo.getBoolAttribute (isPublicTag);
}

void setPresetID (chowdsp::Preset& preset, const String& presetID)
{
    preset.extraInfo.setAttribute (presetIDTag, presetID);
}

String getPresetID (const chowdsp::Preset& preset)
{
    return preset.extraInfo.getStringAttribute (presetIDTag);
}
} // namespace PresetInfoHelpers
