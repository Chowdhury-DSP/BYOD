#include "UnitTests.h"
#include "gui/toolbar/presets/PresetSearchHelpers.h"

class PresetSearchTest : public UnitTest
{
public:
    PresetSearchTest() : UnitTest ("Preset Search Test")
    {
    }

    static void query (const chowdsp::PresetManager& presetManager, const preset_search::Database& searchDatabase, const std::string& query)
    {
        std::cout << " --- \n";

        auto t1 = std::chrono::steady_clock::now();
        auto res = searchDatabase.search (query);
        auto t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> fp_ms = t2 - t1;

        std::cout << "Query: \"" << query << "\" Time: " << fp_ms.count() << "ms #Results: " << res.size() << "\n";

        for (size_t i = 0; i < res.size(); ++i)
        {
            auto& a = res[i];
            std::cout << "    #" << i << ": score: " << a.score << " -> Key: \"" << a.key << "\" -> Preset: " << presetManager.getPresetMap().at (a.key).getName() << "\n";

            // skip after N
            if (i > 22)
                break;
        }
    }

    void runTest() override
    {
        BYOD plugin;
        const auto& presetMgr = plugin.getPresetManager();
        preset_search::Database searchDatabase;

        beginTest ("Initialise Database");
        preset_search::initialiseDatabase (presetMgr, searchDatabase);

        beginTest ("Search");
        query (presetMgr, searchDatabase, "");
        query (presetMgr, searchDatabase, "CHOW");
        query (presetMgr, searchDatabase, "J");
        query (presetMgr, searchDatabase, "Jim");
        query (presetMgr, searchDatabase, "Jimi");
        query (presetMgr, searchDatabase, "JIMI");
    }
};

[[maybe_unused]] static PresetSearchTest presetSearchTest;
