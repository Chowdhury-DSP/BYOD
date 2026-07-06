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
add_subdirectory(ea_variant)
add_subdirectory(math_approx)

# add NeuralAudio (for NAM processor). We can't use its top-level CMakeLists
# directly because it re-adds RTNeural and math_approx via add_subdirectory,
# which would conflict with the targets already defined above. Instead we
# build NeuralAudio's sources ourselves and reuse the deps we already have.
# The NeuralAudio processor is not built on iOS.
if(NOT IOS)
    set(NEURAL_AUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NeuralAudio)
    set(NAM_CORE_DIR ${NEURAL_AUDIO_DIR}/deps/NeuralAmpModelerCore)

    set(NEURAL_AUDIO_SOURCES
        ${NEURAL_AUDIO_DIR}/NeuralAudio/NeuralModel.cpp
        ${NEURAL_AUDIO_DIR}/NeuralAudio/RTNeuralLoader.cpp
    )

    set(NAM_CORE_SOURCES
        ${NAM_CORE_DIR}/NAM/activations.cpp
        ${NAM_CORE_DIR}/NAM/conv1d.cpp
        ${NAM_CORE_DIR}/NAM/get_dsp.cpp
        ${NAM_CORE_DIR}/NAM/ring_buffer.cpp
        ${NAM_CORE_DIR}/NAM/lstm.cpp
        ${NAM_CORE_DIR}/NAM/dsp.cpp
        ${NAM_CORE_DIR}/NAM/container.cpp
        ${NAM_CORE_DIR}/NAM/wavenet/slimmable.cpp
        ${NAM_CORE_DIR}/NAM/wavenet/model.cpp
        ${NAM_CORE_DIR}/NAM/wavenet/a2_fast.cpp
    )

    add_library(NeuralAudio STATIC ${NEURAL_AUDIO_SOURCES} ${NAM_CORE_SOURCES})
    target_compile_features(NeuralAudio PUBLIC cxx_std_20)
    # Note: these definitions are PRIVATE so they don't leak into consumers
    # (BYOD's own code uses a different RTNeural backend).
    target_compile_definitions(NeuralAudio PRIVATE
        NAM_SAMPLE_FLOAT
        DSP_SAMPLE_FLOAT
        BUILD_NAMCORE
        NAM_ENABLE_A2_FAST
        BUILD_INTERNAL_STATIC_WAVENET
        WAVENET_MATH=FastMath
        LSTM_MATH=FastMath
        WAVENET_MAX_NUM_FRAMES=64
        LAYER_ARRAY_BUFFER_PADDING=24
        RTNEURAL_USE_EIGEN=1
        RTNEURAL_DEFAULT_ALIGNMENT=16
        _USE_MATH_DEFINES
    )
    target_include_directories(NeuralAudio
        PUBLIC
            ${NEURAL_AUDIO_DIR}
        PRIVATE
            ${NAM_CORE_DIR}
            ${NEURAL_AUDIO_DIR}/deps/RTNeural
            ${NEURAL_AUDIO_DIR}/deps/RTNeural/modules/Eigen
            ${NEURAL_AUDIO_DIR}/deps/RTNeural/modules/json
            ${NEURAL_AUDIO_DIR}/deps/math_approx/include
    )
    set_target_properties(NeuralAudio PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE
        VISIBILITY_INLINES_HIDDEN TRUE
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
    )
    if (MSVC)
        target_compile_options(NeuralAudio PRIVATE /bigobj)
    endif()
endif()

include(cmake/WarningFlags.cmake)
include(cmake/ConfigFlags.cmake)
include(cmake/RuntimeSIMDLib.cmake)
add_library(juce_plugin_modules STATIC)
target_link_libraries(juce_plugin_modules
    PRIVATE
        BinaryData
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        chowdsp::chowdsp_compressor
        chowdsp::chowdsp_dsp_utils
        chowdsp::chowdsp_eq
        chowdsp::chowdsp_sources
        chowdsp::chowdsp_gui
        chowdsp::chowdsp_plugin_base
        chowdsp::chowdsp_plugin_utils
        chowdsp::chowdsp_reverb
        chowdsp::chowdsp_rhythm
        chowdsp::chowdsp_presets
        chowdsp::chowdsp_serialization
        chowdsp::chowdsp_logging
        chowdsp::chowdsp_units
        chowdsp::chowdsp_fuzzy_search
        chowdsp::chowdsp_wdf
        chowdsp::chowdsp_visualizers
        RTNeural
        ea_variant
        math_approx
    PUBLIC
        config_flags
        juce::juce_recommended_lto_flags
        warning_flags
)

if(BYOD_BUILD_CLAP)
    add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)
    target_link_libraries(juce_plugin_modules
        PRIVATE
            chowdsp::chowdsp_clap_extensions
            clap_juce_extensions
    )
endif()

if(NOT IOS)
    # WHOLE_ARCHIVE is required so that NAMCore's static ConfigParserHelper
    # initializers (in wavenet/model.cpp, lstm.cpp, convnet.cpp, linear.cpp,
    # container.cpp) survive the final link. Without it, the linker discards
    # those object files (they only contain a static registration and are not
    # referenced by symbol), and runtime lookup fails with:
    #   "No config parser registered for architecture: WaveNet"
    # The generator expression propagates the flag to the final executable /
    # shared library link, as required for it to actually take effect.
    target_link_libraries(juce_plugin_modules PRIVATE
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,NeuralAudio>")
endif()

target_compile_definitions(juce_plugin_modules
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_JACK=1
        JUCE_ALSA=1
        JucePlugin_Manufacturer="chowdsp"
        JucePlugin_VersionString="${CMAKE_PROJECT_VERSION}"
        JucePlugin_Name="${CMAKE_PROJECT_NAME}"
        JUCE_MODAL_LOOPS_PERMITTED=1
        CHOWDSP_BUFFER_MAX_NUM_CHANNELS=16
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,COMPILE_DEFINITIONS>
)

add_diagnostic_info(juce_plugin_modules)

target_include_directories(juce_plugin_modules
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,INCLUDE_DIRECTORIES>
    PUBLIC
        ${CMAKE_SOURCE_DIR}/modules/magic_enum/include
        ${CMAKE_SOURCE_DIR}/modules/RTNeural/modules/xsimd/include
        $<TARGET_PROPERTY:sst-cpputils,INTERFACE_INCLUDE_DIRECTORIES>
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

option(BYOD_PERFETTO_PROFILE "Enable profiling with Perfetto" OFF)
if(BYOD_PERFETTO_PROFILE)
    message(STATUS "Enabling Perfetto...")
    add_subdirectory(melatonin_perfetto)
    target_link_libraries(juce_plugin_modules PRIVATE Melatonin::Perfetto)
    target_compile_definitions(juce_plugin_modules PUBLIC PERFETTO=1)
endif()
