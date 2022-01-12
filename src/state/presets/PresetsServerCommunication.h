#pragma once

#include <pch.h>

namespace PresetsServerCommunication
{
enum class CommType
{
    register_user,
    validate_user,
    add_preset,
    get_presets,
};

juce::String sendServerRequest (CommType type, const juce::String& user, const juce::String& pass, const juce::String& presetName = {});

} // namespace PresetsServerCommunication
