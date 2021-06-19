cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment target")
project(BYOD VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
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

juce_add_plugin(BYOD
    COMPANY_NAME chowdsp
    PLUGIN_MANUFACTURER_CODE Chow
    PLUGIN_CODE dqu0
    FORMATS ${JUCE_FORMATS}
    ProductName "BYOD"
    LV2_URI https://github.com/Chowdhury-DSP/BYOD
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
option(MACOS_RELEASE "Set build flags for MacOS Release" OFF)
if(MACOS_RELEASE)
    message(STATUS "Setting MacOS release flags...")
    set_target_properties(BYOD_Standalone PROPERTIES
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES)
endif()
