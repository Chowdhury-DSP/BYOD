if(NOT LINUX)
    if(WIN32)
        set(JAI_COMPILER_EXE "jai.exe")
    elseif(APPLE)
        set(JAI_COMPILER_EXE "jai-macos")
    else()
        set(JAI_COMPILER_EXE "jai-linux")
    endif()

    find_program(JAI_COMPILER
        NAMES ${JAI_COMPILER_EXE}
        HINTS ${CMAKE_SOURCE_DIR}/modules/jai/bin ${CMAKE_SOURCE_DIR}/../../Research/jai/bin
    )
    message(STATUS "Jai compiler: ${JAI_COMPILER}")
else()
    message(STATUS "Skipping Jai checks on this platform")
    set(JAI_COMPILER "JAI_COMPILER-NOTFOUND")
endif()
