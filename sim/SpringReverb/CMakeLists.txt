cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
project(spring_reverb VERSION 1.0.0)

add_subdirectory(../../modules/JUCE JUCE)
juce_add_modules(../../modules/chowdsp_utils)

juce_add_console_app(spring_reverb
    COMPANY_NAME chowdsp
)

juce_generate_juce_header(spring_reverb)
add_custom_command(TARGET spring_reverb
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E echo "copying $<TARGET_FILE:spring_reverb> to ${PROJECT_BINARY_DIR}/spring_reverb"
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:spring_reverb> ${PROJECT_BINARY_DIR}/spring_reverb)

target_sources(spring_reverb PRIVATE
    main.cpp
    ../../src/processors/other/spring_reverb/SpringReverb.cpp
)

target_compile_definitions(spring_reverb
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        CHOWDSP_USE_FOLEYS_CLASSES=0
        JucePlugin_Name="spring_reverb"
        JucePlugin_Manufacturer="chowdsp"
        JucePlugin_VersionString="1.0.0"
)

target_include_directories(spring_reverb
    PRIVATE
        .
        ../../src/processors/other/spring_reverb
)

target_link_libraries(spring_reverb
    PRIVATE
        chowdsp_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

set_target_properties(spring_reverb PROPERTIES CXX_VISIBILITY_PRESET hidden)
