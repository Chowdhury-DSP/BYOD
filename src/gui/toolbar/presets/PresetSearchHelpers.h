#pragma once

#include <pch.h>

namespace preset_search
{
using Database = fuzzysearch::Database<int>;
using Results = std::vector<const chowdsp::Preset*>;

void initialiseDatabase (const chowdsp::PresetManager& presetManager, Database& database);
Results getSearchResults (const chowdsp::PresetManager& presetManager, const Database& database, const juce::String& query);
} // namespace preset_search
