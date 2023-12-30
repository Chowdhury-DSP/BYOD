#if HAS_CLAP_JUCE_EXTENSIONS

#include "PresetDiscovery.h"
#include "PresetManager.h"
#include "processors/ProcessorStore.h"
#include "processors/chain/ProcessorChain.h"

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wunused-parameter")
JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4100)
#include <clap/helpers/preset-discovery-provider.hh>
#include <clap/helpers/preset-discovery-provider.hxx>
JUCE_END_IGNORE_WARNINGS_MSVC
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

namespace preset_discovery
{
static constexpr clap_plugin_id plugin_id {
    .abi = "clap",
    .id = BYOD_CLAP_ID,
};

struct FactoryPresetsProvider
#if JUCE_DEBUG
    : clap::helpers::PresetDiscoveryProvider<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>
#else
    : clap::helpers::PresetDiscoveryProvider<clap::helpers::MisbehaviourHandler::Ignore, clap::helpers::CheckingLevel::Minimal>
#endif
{
    static constexpr clap_preset_discovery_provider_descriptor descriptor {
        .clap_version = CLAP_VERSION_INIT,
        .id = "org.chowdsp.byod.factory-presets",
        .name = "BYOD Factory Presets Provider",
        .vendor = "ChowDSP"
    };

    explicit FactoryPresetsProvider (const clap_preset_discovery_indexer* indexer)
        : PresetDiscoveryProvider (&descriptor, indexer)
    {
    }

    static constexpr clap_preset_discovery_location factoryPresetsLocation {
        .flags = CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT,
        .name = " BYOD Factory Presets Location",
        .kind = CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN,
        .location = nullptr,
    };

    bool init() noexcept override
    {
        indexer()->declare_location (indexer(), &factoryPresetsLocation);
        return true;
    }

    bool getMetadata (uint32_t location_kind,
                      [[maybe_unused]] const char* location,
                      const clap_preset_discovery_metadata_receiver_t* metadata_receiver) noexcept override
    {
        if (location_kind != CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN)
            return false;

        ScopedJuceInitialiser_GUI scopedJuce {};
        const ProcessorStore procStore { nullptr };
        for (const auto& factoryPreset : PresetManager::getFactoryPresets (procStore))
        {
            DBG ("Indexing factory preset: " + factoryPreset.getName());
            if (metadata_receiver->begin_preset (metadata_receiver, factoryPreset.getName().toRawUTF8(), factoryPreset.getName().toRawUTF8()))
            {
                metadata_receiver->add_plugin_id (metadata_receiver, &plugin_id);
                metadata_receiver->add_creator (metadata_receiver, factoryPreset.getVendor().toRawUTF8());

                if (factoryPreset.getCategory().isNotEmpty())
                    metadata_receiver->add_feature (metadata_receiver, factoryPreset.getCategory().toRawUTF8());
            }
            else
            {
                break;
            }
        }

        return true;
    }
};

static auto getUserPresetsFolder()
{
    return chowdsp::PresetManager::getUserPresetPath (chowdsp::toString (PresetManager::userPresetPath));
}

struct UserPresetsProvider
#if JUCE_DEBUG
    : clap::helpers::PresetDiscoveryProvider<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>
#else
    : clap::helpers::PresetDiscoveryProvider<clap::helpers::MisbehaviourHandler::Ignore, clap::helpers::CheckingLevel::Minimal>
#endif
{
    static constexpr clap_preset_discovery_provider_descriptor descriptor {
        .clap_version = CLAP_VERSION_INIT,
        .id = "org.chowdsp.byod.user-presets",
        .name = "BYOD User Presets Provider",
        .vendor = "User"
    };

    explicit UserPresetsProvider (const clap_preset_discovery_indexer* indexer)
        : PresetDiscoveryProvider (&descriptor, indexer)
    {
    }

    static constexpr clap_preset_discovery_filetype filetype {
        .name = "User Preset Filetype",
        .description = "User preset filetype for BYOD",
        .file_extension = "chowpreset",
    };

    juce::File userPresetsFolder {};
    clap_preset_discovery_location userPresetsLocation {};

    bool init() noexcept override
    {
        indexer()->declare_filetype (indexer(), &filetype);

        userPresetsFolder = getUserPresetsFolder();
        if (userPresetsFolder == juce::File {} || ! userPresetsFolder.isDirectory())
            return false;

        userPresetsLocation.flags = CLAP_PRESET_DISCOVERY_IS_USER_CONTENT;
        userPresetsLocation.kind = CLAP_PRESET_DISCOVERY_LOCATION_FILE;
        userPresetsLocation.name = "BYOD User Presets Location";
        userPresetsLocation.location = userPresetsFolder.getFullPathName().toRawUTF8();
        indexer()->declare_location (indexer(), &userPresetsLocation);

        return true;
    }

    bool getMetadata (uint32_t location_kind,
                      const char* location,
                      const clap_preset_discovery_metadata_receiver_t* metadata_receiver) noexcept override
    {
        if (location_kind != CLAP_PRESET_DISCOVERY_LOCATION_FILE || location == nullptr)
            return false;

        const auto userPresetFile = juce::File { location };
        if (! userPresetFile.existsAsFile())
            return false;

        chowdsp::Preset preset { userPresetFile };
        if (! preset.isValid())
            return false;

        if (metadata_receiver->begin_preset (metadata_receiver, userPresetFile.getFullPathName().toRawUTF8(), ""))
        {
            metadata_receiver->add_plugin_id (metadata_receiver, &plugin_id);
            metadata_receiver->add_creator (metadata_receiver, preset.getVendor().toRawUTF8());

            if (preset.getCategory().isNotEmpty())
                metadata_receiver->add_feature (metadata_receiver, preset.getCategory().toRawUTF8());
            metadata_receiver->set_timestamps (metadata_receiver,
                                               (clap_timestamp_t) userPresetFile.getCreationTime().toMilliseconds() / 1000,
                                               (clap_timestamp_t) userPresetFile.getLastModificationTime().toMilliseconds() / 1000);
        }

        return true;
    }
};

uint32_t count (const struct clap_preset_discovery_factory*)
{
    const auto userPresetsFolder = getUserPresetsFolder();
    if (userPresetsFolder == juce::File {} || ! userPresetsFolder.isDirectory())
        return 1;
    return 2;
}

const clap_preset_discovery_provider_descriptor_t* get_descriptor (
    const struct clap_preset_discovery_factory*,
    uint32_t index)
{
    if (index == 0)
        return &FactoryPresetsProvider::descriptor;

    if (index == 1)
        return &UserPresetsProvider::descriptor;

    return nullptr;
}

const clap_preset_discovery_provider_t* create (
    const struct clap_preset_discovery_factory*,
    const clap_preset_discovery_indexer_t* indexer,
    const char* provider_id)
{
    if (strcmp (provider_id, FactoryPresetsProvider::descriptor.id) == 0)
    {
        auto* provider = new FactoryPresetsProvider { indexer };
        return provider->provider();
    }

    if (strcmp (provider_id, UserPresetsProvider::descriptor.id) == 0)
    {
        auto* provider = new UserPresetsProvider { indexer };
        return provider->provider();
    }

    return nullptr;
}

bool presetLoadFromLocation (chowdsp::PresetManager& presetManager, uint32_t location_kind, const char* location, const char* load_key) noexcept
{
    auto& byodPresetManager = static_cast<PresetManager&> (presetManager); // NOLINT

    if (location_kind == CLAP_PRESET_DISCOVERY_LOCATION_FILE)
    {
        const auto presetFile = juce::File { location };
        if (! presetFile.existsAsFile())
        {
            presetManager.loadDefaultPreset();
            return true;
        }

        auto preset = chowdsp::Preset { presetFile };
        if (! preset.isValid())
            return false;

        byodPresetManager.loadPresetSafe (std::make_unique<chowdsp::Preset> (std::move (preset)), nullptr);

        return true;
    }

    if (location_kind == CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN)
    {
        for (auto& preset : byodPresetManager.getFactoryPresets (byodPresetManager.getProcessorChain()->getProcStore()))
        {
            if (preset.getName() == load_key)
            {
                byodPresetManager.loadPresetSafe (std::make_unique<chowdsp::Preset> (std::move (preset)), nullptr);
                return true;
            }
        }
    }

    return false;
}
} // namespace preset_discovery

#endif // HAS_CLAP_JUCE_EXTENSIONS
