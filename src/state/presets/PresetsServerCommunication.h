#pragma once

#include <pch.h>

namespace PresetsServerCommunication
{
enum class CommType
{
    register_user,
    validate_user,
    add_preset,
    update_preset,
    get_presets,
};

juce::String sendServerRequest (CommType type, const juce::String& user, const juce::String& pass);

struct PresetRequestInfo
{
    const juce::String user;
    const juce::String pass;
    const juce::String presetName;
    const juce::String presetData;
    const bool isPublic;
    const juce::String presetID {};
};

juce::String sendAddPresetRequest (const PresetRequestInfo& info);
juce::String sendUpdatePresetRequest (const PresetRequestInfo& info);

juce::String parseMessageResponse (const String& messageResponse);

} // namespace PresetsServerCommunication
