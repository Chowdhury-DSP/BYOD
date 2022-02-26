add_subdirectory(JUCE)
include(cmake/SubprojectVersion.cmake)
subproject_version(JUCE juce_version)
message(STATUS "VERSION for JUCE: ${juce_version}")

# add custom JUCE modules
juce_add_modules(foleys_gui_magic)
add_subdirectory(chowdsp_utils)
juce_add_modules(chowdsp_juce_dsp)
add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)

# add RTNeural
if(APPLE)
    set(RTNEURAL_EIGEN ON CACHE BOOL "Use RTNeural with Eigen backend" FORCE)
else()
    set(RTNEURAL_XSIMD ON CACHE BOOL "Use RTNeural with XSIMD backend" FORCE)
endif()
add_subdirectory(RTNeural)

# add sst-cpputils
add_subdirectory(sst-cpputils)

# add rapidfuzz
add_subdirectory(rapidfuzz-cpp)

include(cmake/WarningFlags.cmake)
include(cmake/ConfigFlags.cmake)
add_library(juce_plugin_modules STATIC)
target_link_libraries(juce_plugin_modules
    PRIVATE
        BinaryData
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        chowdsp_juce_dsp
        chowdsp_dsp
        chowdsp_gui
        chowdsp_plugin_utils
        foleys_gui_magic
        RTNeural
        rapidfuzz::rapidfuzz
    PUBLIC
        config_flags
        juce::juce_recommended_lto_flags
        warning_flags
)

target_compile_definitions(juce_plugin_modules
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_JACK=1
        JUCE_ALSA=1
        CHOWDSP_USE_XSIMD=1
        CHOWDSP_USE_CUSTOM_JUCE_DSP=1
        JucePlugin_Manufacturer="chowdsp"
        JucePlugin_VersionString="${CMAKE_PROJECT_VERSION}"
        JucePlugin_Name="${CMAKE_PROJECT_NAME}"
        JUCE_MODAL_LOOPS_PERMITTED=1
        FOLEYS_SHOW_GUI_EDITOR_PALLETTE=0
        FOLEYS_ENABLE_BINARY_DATA=1
        FOLEYS_ENABLE_OPEN_GL_CONTEXT=0 # Foley's editor should NOT use OpenGL by default
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,COMPILE_DEFINITIONS>
)

target_include_directories(juce_plugin_modules
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,INCLUDE_DIRECTORIES>
    PUBLIC
        ${CMAKE_SOURCE_DIR}/modules/magic_enum/include
        ${CMAKE_SOURCE_DIR}/modules/RTNeural
        ${CMAKE_SOURCE_DIR}/modules/RTNeural/modules/Eigen
        ${CMAKE_SOURCE_DIR}/modules/RTNeural/modules/xsimd/include
        $<TARGET_PROPERTY:sst-cpputils,INTERFACE_INCLUDE_DIRECTORIES>
        ${CMAKE_SOURCE_DIR}/modules/rapidfuzz-cpp
)

set_target_properties(juce_plugin_modules PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_INLINES_HIDDEN TRUE
    C_VISBILITY_PRESET hidden
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

if(NOT APPLE)
    message(STATUS "Linking with OpenGL")
    target_link_libraries(juce_plugin_modules PRIVATE juce::juce_opengl)
endif()
