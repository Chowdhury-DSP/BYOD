add_library(config_flags INTERFACE)

if((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    target_compile_options(juce_recommended_config_flags INTERFACE
            $<IF:$<CONFIG:Debug>,/Od /Zi, /O2 /fp:fast> $<$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">:/MP> /EHsc)
elseif((CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    target_compile_options(juce_recommended_config_flags INTERFACE
            $<$<CONFIG:Debug>:-g -O0>
            $<$<CONFIG:Release>:-Ofast>)
endif()
