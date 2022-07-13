add_subdirectory(JUCE)

# add custom JUCE modules
add_subdirectory(chowdsp_utils)
add_subdirectory(chowdsp_wdf)

subproject_version(JUCE juce_version)
message(STATUS "VERSION for JUCE: ${juce_version}")
subproject_version(chowdsp_utils chowdsp_version)
message(STATUS "VERSION for chowdsp_utils: ${chowdsp_version}")

# add RTNeural
set(RTNEURAL_XSIMD ON CACHE BOOL "Use RTNeural with XSIMD backend" FORCE)
add_subdirectory(RTNeural)

 # add other C++ libraries
add_subdirectory(sst-cpputils)
add_subdirectory(rapidfuzz-cpp)

include(cmake/WarningFlags.cmake)
include(cmake/ConfigFlags.cmake)
add_library(juce_plugin_modules STATIC)
target_link_libraries(juce_plugin_modules
    PRIVATE
        BinaryData
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        chowdsp::chowdsp_dsp_utils
        chowdsp::chowdsp_gui
        chowdsp::chowdsp_plugin_base
        chowdsp::chowdsp_plugin_utils
        chowdsp::chowdsp_presets
        chowdsp_wdf
        RTNeural
        rapidfuzz::rapidfuzz
    PUBLIC
        config_flags
        juce::juce_recommended_lto_flags
        warning_flags
)

if(NOT IOS AND BYOD_BUILD_CLAP)
    add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)
    target_link_libraries(juce_plugin_modules
        PRIVATE
            chowdsp::chowdsp_clap_extensions
            clap_juce_extensions
    )
endif()

target_compile_definitions(juce_plugin_modules
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_JACK=1
        JUCE_ALSA=1
        JUCE_COREGRAPHICS_DRAW_ASYNC=1
        JucePlugin_Manufacturer="chowdsp"
        JucePlugin_VersionString="${CMAKE_PROJECT_VERSION}"
        JucePlugin_Name="${CMAKE_PROJECT_NAME}"
        JUCE_MODAL_LOOPS_PERMITTED=1
        CHOWDSP_USING_JUCE=1
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,COMPILE_DEFINITIONS>
)

add_diagnostic_info(juce_plugin_modules)

target_include_directories(juce_plugin_modules
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,INCLUDE_DIRECTORIES>
    PUBLIC
        ${CMAKE_SOURCE_DIR}/modules/magic_enum/include
        ${CMAKE_SOURCE_DIR}/modules/RTNeural/modules/Eigen
        ${CMAKE_SOURCE_DIR}/modules/RTNeural/modules/xsimd/include
        $<TARGET_PROPERTY:sst-cpputils,INTERFACE_INCLUDE_DIRECTORIES>
        ${CMAKE_SOURCE_DIR}/modules/rapidfuzz-cpp
)

set_target_properties(juce_plugin_modules PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_INLINES_HIDDEN TRUE
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
)

if(ASIOSDK_DIR)
    message(STATUS "Using ASIO SDK from ${ASIOSDK_DIR}")
    target_include_directories(juce_plugin_modules PUBLIC ${ASIOSDK_DIR}/common)
    target_compile_definitions(juce_plugin_modules PUBLIC JUCE_ASIO=1)
endif()

if(IOS)
    target_link_libraries(juce_plugin_modules PRIVATE juce::juce_product_unlocking)
    target_compile_definitions(juce_plugin_modules PUBLIC JUCE_IN_APP_PURCHASES=1)
endif()

# Enable OpenGL on Windows by default.
if (WIN32)
    option(CHOWDSP_ENABLE_OPENGL "Enable OpenGL for this plugin" ON)
else()
    option(CHOWDSP_ENABLE_OPENGL "Enable OpenGL for this plugin" OFF)
endif()

if(CHOWDSP_ENABLE_OPENGL)
    message(STATUS "Linking with OpenGL")
    target_link_libraries(juce_plugin_modules PRIVATE juce::juce_opengl)
endif()
