#include "PresetSearchHelpers.h"

namespace preset_search
{
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

    database.reset();
    database.setWeights ({
        1.0f, // Name
        0.9f, // Vendor
        1.0f, // Category
    });
    database.setThreshold (0.5f);

    const auto t1 = std::chrono::steady_clock::now();
    std::vector<std::string> fields (magic_enum::enum_count<SearchFields>());

    for (const auto& [presetID, preset] : presetManager.getPresetMap())
    {
        fields[Name] = preset.getName().toStdString();
        fields[Vendor] = preset.getVendor().toStdString();
        fields[Category] = preset.getCategory().toStdString();
        database.addEntry (presetID, fields); // TODO: it would be cool if we could add entries in a batch?
    }

    const auto t2 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
    juce::Logger::writeToLog ("Finished scanning preset database in "
                              + juce::String { fp_ms.count() }
                              + " milliseconds");
}

Results getSearchResults (const chowdsp::PresetManager& presetManager, const Database& database, const juce::String& query)
{
    Results resultsVector;
    const auto& presetMap = presetManager.getPresetMap();
    for (const auto result : database.search (query.toStdString()))
    {
        const auto presetIter = presetMap.find (result.key);
        if (presetIter != presetMap.end())
            resultsVector.push_back (&presetIter->second);
    }
    return resultsVector;
}
} // namespace preset_search
