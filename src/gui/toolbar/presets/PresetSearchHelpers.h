#pragma once

#include <pch.h>

namespace preset_search
{
static constexpr size_t numPresetSearchFields = 3;
using Database = chowdsp::SearchDatabase<int, numPresetSearchFields>;
using Results = std::vector<const chowdsp::Preset*>;

void initialiseDatabase (const chowdsp::PresetManager& presetManager, Database& database);
Results getSearchResults (const chowdsp::PresetManager& presetManager, const Database& database, const juce::String& query);
} // namespace preset_search
