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
    ScreenshotGenerator.cpp

    UnitTests/PreBufferTest.cpp
    UnitTests/UnitTests.cpp
)

target_precompile_headers(BYOD_headless PRIVATE ../pch.h)
get_target_property(juce_includes juce_plugin_modules INCLUDE_DIRECTORIES)
message(STATUS "${juce_includes}")
target_include_directories(BYOD_headless
    PRIVATE
    ../
    ../../modules
    ../../modules/JUCE/modules
    ${juce_includes}
)

target_link_libraries(BYOD_headless PUBLIC
    BinaryData
    BYOD
)

set_target_properties(BYOD_headless PROPERTIES CXX_VISIBILITY_PRESET hidden)
