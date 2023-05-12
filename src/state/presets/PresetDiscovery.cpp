#if HAS_CLAP_JUCE_EXTENSIONS

#include "PresetDiscovery.h"
#include "PresetManager.h"

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wunused-parameter")
#include <clap/helpers/preset-discovery-provider.hh>
#include <clap/helpers/preset-discovery-provider.hxx>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

namespace preset_discovery
{
static constexpr clap_plugin_id plugin_id {
    .abi = "clap",
    .id = CLAP_ID,
};

struct UserPresetsProvider
#if JUCE_DEBUG
    : clap::helpers::PresetDiscoveryProvider<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>
#else
    : clap::helpers::PresetDiscoveryProvider<clap::helpers::MisbehaviourHandler::Ignore, clap::helpers::CheckingLevel::Minimal>
#endif
{
    static constexpr clap_preset_discovery_provider_descriptor descriptor {
        .clap_version = CLAP_VERSION,
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

        userPresetsFolder = chowdsp::PresetManager::getUserPresetPath (chowdsp::toString (PresetManager::userPresetPath));
        if (! userPresetsFolder.isDirectory())
            return false;

        userPresetsLocation.flags = CLAP_PRESET_DISCOVERY_IS_USER_CONTENT;
        userPresetsLocation.kind = CLAP_PRESET_DISCOVERY_LOCATION_FILE;
        userPresetsLocation.name = "User Presets Location";
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
        }

        return true;
    }
};

uint32_t count (const struct clap_preset_discovery_factory*)
{
    return 1;
}

const clap_preset_discovery_provider_descriptor_t* get_descriptor (
    const struct clap_preset_discovery_factory*,
    uint32_t index)
{
    if (index == 0)
        return &UserPresetsProvider::descriptor;

    return nullptr;
}

const clap_preset_discovery_provider_t* create (
    const struct clap_preset_discovery_factory*,
    const clap_preset_discovery_indexer_t* indexer,
    const char* provider_id)
{
    if (strcmp (provider_id, UserPresetsProvider::descriptor.id) == 0)
    {
        auto* provider = new UserPresetsProvider { indexer };
        return provider->provider();
    }

    return nullptr;
}

bool presetLoadFromLocation (chowdsp::PresetManager& presetManager, uint32_t location_kind, const char* location, const char* load_key) noexcept
{
    juce::ignoreUnused (load_key);
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

        static_cast<PresetManager&> (presetManager).loadPresetSafe (std::make_unique<chowdsp::Preset> (std::move (preset)), nullptr); //NOLINT

        return true;
    }

    return false;
}
} // namespace preset_discovery

#endif // HAS_CLAP_JUCE_EXTENSIONS
