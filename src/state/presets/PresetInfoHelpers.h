#if BYOD_BUILD_PRESET_SERVER

#pragma once

#include <pch.h>

namespace PresetInfoHelpers
{
void setIsPublic (chowdsp::Preset& preset, bool shouldBePublic);
bool getIsPublic (const chowdsp::Preset& preset);

void setPresetID (chowdsp::Preset& preset, const String& presetID);
String getPresetID (const chowdsp::Preset& preset);

} // namespace PresetInfoHelpers

#endif // BYOD_BUILD_PRESET_SERVER
