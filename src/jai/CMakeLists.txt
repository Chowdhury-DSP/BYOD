if(WIN32)
    set(JAI_LIBRARY_FILE "byod_jai_lib.lib")
    set(JAI_TARGET_OS "WINDOWS")
elseif(IOS)
    set(JAI_LIBRARY_FILE "byod_jai_lib_ios.a")
    set(JAI_TARGET_OS "IOS")
else()
    set(JAI_LIBRARY_FILE "byod_jai_lib.a")
    if(APPLE)
        set(JAI_TARGET_OS "MACOS")
    else()
        set(JAI_TARGET_OS "LINUX")
    endif()
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${JAI_LIBRARY_FILE}
    VERBATIM
    COMMAND ${JAI_COMPILER} build.jai -add "TARGET_OS :: \"${JAI_TARGET_OS}\""
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
target_sources(BYOD PRIVATE SharedJaiContext.cpp)
