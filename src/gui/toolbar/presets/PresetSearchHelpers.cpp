#include "PresetSearchHelpers.h"

namespace preset_search
{
static std::string_view to_string_view (const juce::String& str) noexcept
{
    return { str.toRawUTF8(), str.getNumBytesAsUTF8() };
}

void initialiseDatabase (const chowdsp::PresetManager& presetManager, Database& database)
{
    enum SearchFields
    {
        Name = 0,
        Vendor,
        Category,
        // TODO: maybe add a field for the processors present in the preset?
    };

    juce::Logger::writeToLog ("Initializing preset search database...");

    database.resetEntries (presetManager.getNumPresets());
    database.setWeights ({
        1.0f, // Name
        0.9f, // Vendor
        1.0f, // Category
    });
    database.setThreshold (0.5f);

    const auto t1 = std::chrono::steady_clock::now();
    std::array<std::string_view, numPresetSearchFields> fields {};

    for (const auto& [presetID, preset] : presetManager.getPresetMap())
    {
        fields[Name] = to_string_view (preset.getName());
        fields[Vendor] = to_string_view (preset.getVendor());
        fields[Category] = to_string_view (preset.getCategory());
        database.addEntry (presetID, fields);
    }

    const auto t2 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
    juce::Logger::writeToLog ("Finished scanning preset database in "
                              + juce::String { fp_ms.count() }
                              + " milliseconds");

    database.prepareForSearch();
}

Results getSearchResults (const chowdsp::PresetManager& presetManager, const Database& database, const juce::String& query)
{
    const auto searchResults = database.search (to_string_view (query));

    Results resultsVector;
    resultsVector.reserve (searchResults.size());

    const auto& presetMap = presetManager.getPresetMap();
    for (const auto& result : searchResults)
    {
        const auto presetIter = presetMap.find (result.key);
        if (presetIter != presetMap.end())
            resultsVector.push_back (&presetIter->second);
    }
    return resultsVector;
}
} // namespace preset_search
