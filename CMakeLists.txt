cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment target")
project(BYOD VERSION 0.5.2)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()

add_subdirectory(modules)

# juce_set_vst2_sdk_path(C:/SDKs/VST_SDK/VST2_SDK)
# set default plugin formats to build
if(IOS)
    set(JUCE_FORMATS Standalone AUv3)
else()
    set(JUCE_FORMATS AU VST3 Standalone)
endif()

# Build LV2 only on Linux
if(UNIX AND NOT APPLE)
    message(STATUS "Building LV2 plugin format")
    list(APPEND JUCE_FORMATS LV2)
endif()

# Build VST2 is SDK set
if(TARGET juce_vst2_sdk)
    message(STATUS "Building VST2 plugin format")
    list(APPEND JUCE_FORMATS VST)
endif()

option(BYOD_USE_LOCAL_PRESET_SERVER "Set build flag to use local preset server" OFF)
if ((NOT IOS) AND BYOD_USE_LOCAL_PRESET_SERVER)
    message(STATUS "Configuring for testing with local preset server")
    include (modules/cmake/AppleLocalNetworkPlist.cmake)
endif()

option(BUILD_RELEASE "Set build flags for release builds" OFF)
if(BUILD_RELEASE)
    set(HARDENED_RUNTIME_ENABLED YES)
else()
    set(HARDENED_RUNTIME_ENABLED NO)
endif()

juce_add_plugin(BYOD
    COMPANY_NAME chowdsp
    PLUGIN_MANUFACTURER_CODE Chow
    PLUGIN_CODE Dqu0
    FORMATS ${JUCE_FORMATS}
    ProductName "BYOD"
    ICON_BIG res/logo.png

    VST2_CATEGORY kPlugCategEffect
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    AAX_CATEGORY AAX_ePlugInCategory_Harmonic
    LV2_URI https://github.com/Chowdhury-DSP/BYOD

    MICROPHONE_PERMISSION_ENABLED TRUE
    NEEDS_STORE_KIT TRUE
    REQUIRES_FULL_SCREEN TRUE
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    PLIST_TO_MERGE ${APPLE_LOCAL_NETWORK_PLIST} # Apple won't let you use local network unless we add this
    ICLOUD_PERMISSIONS_ENABLED TRUE
    HARDENED_RUNTIME_ENABLED ${HARDENED_RUNTIME_ENABLED}
)

option(BYOD_BUILD_CLAP "Build a CLAP plugin target (experimental)" OFF)
if(BYOD_BUILD_CLAP AND NOT IOS)
    clap_juce_extensions_plugin(
        TARGET BYOD
        CLAP_ID "org.chowdsp.byod"
        CLAP_FEATURES distortion audio_effect
    )
endif()

juce_generate_juce_header(BYOD)
add_subdirectory(src)
include_directories(src)
add_subdirectory(res)

include(modules/cmake/VersionInfo.cmake)

target_compile_definitions(BYOD PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0

    BYOD_GIT_BRANCH="${GIT_BRANCH}"
    BYOD_GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
    BYOD_BUILD_FQDN="${PLUGIN_BUILD_FQDN}"
    BYOD_CXX_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}"
    BYOD_CXX_COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}"
)

if ((NOT IOS) AND BYOD_USE_LOCAL_PRESET_SERVER)
    target_compile_definitions(BYOD PRIVATE BYOD_USE_LOCAL_PRESET_SERVER=1)
endif()

target_link_libraries(BYOD PRIVATE juce_plugin_modules)

if(IOS)
    message(STATUS "Setting iOS-specific properties...")

    foreach(target IN ITEMS BinaryData RTNeural juce_plugin_modules BYOD BYOD_Standalone BYOD_AUv3)
        set_target_properties(${target}
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "./"
                ARCHIVE_OUTPUT_DIRECTORY "./"
                LIBRARY_OUTPUT_DIRECTORY "./")
    endforeach()

    set_target_properties(BYOD_Standalone PROPERTIES
            XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
            XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
            XCODE_ATTRIBUTE_ENABLE_IN_APP_PURCHASE "YES")

    set_target_properties(BYOD_AUv3 PROPERTIES
            XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)/BYOD.app/PlugIns"
            XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
            XCODE_ATTRIBUTE_ENABLE_IN_APP_PURCHASE "YES")
endif()
