if(WIN32)
    set(JAI_LIBRARY_FILE "byod_jai_lib.lib")
else()
    set(JAI_LIBRARY_FILE "byod_jai_lib.a")
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${JAI_LIBRARY_FILE}
    COMMAND ${JAI_COMPILER} build.jai
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS build.jai krusher/bit_reduction.jai krusher/lofi_downsampler.jai
)
add_custom_target(jai_library_build DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${JAI_LIBRARY_FILE})

add_library(byod_jai_lib STATIC IMPORTED GLOBAL)
add_dependencies(byod_jai_lib jai_library_build)

set_target_properties(byod_jai_lib
    PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/${JAI_LIBRARY_FILE}
)

target_link_libraries(BYOD PRIVATE byod_jai_lib)
target_sources(BYOD PRIVATE SharedJaiContext.cpp stb_sprintf.cpp)
target_compile_definitions(BYOD PRIVATE STB_SPRINTF_IMPLEMENTATION=1)

if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    string(JOIN " " STB_CXX_FLAGS
        "-Wno-language-extension-token"
        "-Wno-zero-as-null-pointer-constant"
        "-Wno-cast-align"
        "-Wno-implicit-fallthrough"
        "-Wno-conditional-uninitialized"
        "-Wno-duplicate-decl-specifier"
        "-Wno-unreachable-code"
    )
    set_source_files_properties(stb_sprintf.cpp
        TARGET_DIRECTORY BYOD
        PROPERTIES COMPILE_FLAGS "${STB_CXX_FLAGS}"
    )
endif()

#TODO: remove stb_sprintf once Jai.Basic no longer depends on it!
