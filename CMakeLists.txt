cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment target")
project(BYOD VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(modules)

# juce_set_vst2_sdk_path(C:/SDKs/VST_SDK/VST2_SDK)
# set default plugin formats to build
option(BUILD_RELEASE "Set build flags for release builds" OFF)
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

juce_add_plugin(BYOD
    COMPANY_NAME chowdsp
    PLUGIN_MANUFACTURER_CODE Chow
    PLUGIN_CODE Dqu0
    FORMATS ${JUCE_FORMATS}
    ProductName "BYOD"
    ICON_BIG res/logo.png

    LV2_URI https://github.com/Chowdhury-DSP/BYOD

    MICROPHONE_PERMISSION_ENABLED TRUE
    NEEDS_STORE_KIT TRUE
    REQUIRES_FULL_SCREEN TRUE
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
)

juce_generate_juce_header(BYOD)
add_subdirectory(src)
include_directories(src)
add_subdirectory(res)

target_compile_definitions(BYOD PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BYOD PRIVATE juce_plugin_modules)

# we need these flags for notarization on MacOS
if(BUILD_RELEASE AND APPLE)
    message(STATUS "Setting MacOS release flags...")
    set_target_properties(BYOD_Standalone PROPERTIES
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES)
endif()

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
