juce_add_console_app(BYOD_headless
    PRODUCT_NAME "BYOD"
    COMPANY_NAME chowdsp
)

juce_generate_juce_header(BYOD_headless)
add_custom_command(TARGET BYOD_headless
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E echo "copying $<TARGET_FILE:BYOD_headless> to ${PROJECT_BINARY_DIR}/BYOD"
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:BYOD_headless> ${PROJECT_BINARY_DIR}/BYOD)

target_sources(BYOD_headless PRIVATE
    main.cpp
    PresetResaver.cpp
    PresetSaveLoadTime.cpp
    ScreenshotGenerator.cpp
    GuitarMLFilterDesigner.cpp

    tests/AmpIRsSaveLoadTest.cpp
    tests/BadModulationTest.cpp
    tests/ForwardingParamStabilityTest.cpp
    tests/NaNResetTest.cpp
    tests/ParameterSmoothTest.cpp
    tests/PreBufferTest.cpp
    tests/PresetsTest.cpp
    tests/PresetSearchTest.cpp
    tests/ProcessorStoreInfoTest.cpp
    tests/RAMUsageTest.cpp
    tests/SilenceTest.cpp
    tests/StereoTest.cpp
    tests/UndoRedoTest.cpp
    tests/UnitTests.cpp
    tests/WaveshaperTest.cpp
)

target_precompile_headers(BYOD_headless PRIVATE ../pch.h)
get_target_property(juce_includes juce_plugin_modules INCLUDE_DIRECTORIES)
target_include_directories(BYOD_headless
    PRIVATE
        ../
        ../../modules
        ../../modules/RTNeural
        ../../modules/chowdsp_utils/modules/dsp
        ../../modules/chowdsp_utils/modules/gui
        ../../modules/chowdsp_utils/modules/plugin
        ../../modules/chowdsp_utils/modules/common
        ../../modules/chowdsp_utils/modules/music
        ../../modules/JUCE/modules
        ../../modules/chowdsp_wdf/include
        ../../modules/ea_variant
        ../../modules/math_approx/include
        ${juce_includes}
)

target_link_libraries(BYOD_headless PUBLIC
    BinaryData
    BYOD
)

target_compile_definitions(BYOD_headless
    PRIVATE
        BYOD_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

set_target_properties(BYOD_headless PROPERTIES CXX_VISIBILITY_PRESET hidden)
