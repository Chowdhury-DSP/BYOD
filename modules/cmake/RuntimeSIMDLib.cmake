function(make_lib_simd_runtime name file)
    add_library(${name}_sse_or_arm STATIC)
    target_sources(${name}_sse_or_arm PRIVATE ${file})

    add_library(${name}_avx STATIC)
    target_sources(${name}_avx PRIVATE ${file})
    target_compile_definitions(${name}_avx PRIVATE BYOD_COMPILING_WITH_AVX=1)
    if(WIN32)
        target_compile_options(${name}_avx PRIVATE /arch:AVX)
    else()
        # @TODO: can we exclude this for the ARM build altogether?
        target_compile_options(${name}_avx PRIVATE -mavx -mfma -Wno-unused-command-line-argument)
    endif()

    add_library(${name} INTERFACE)
    target_link_libraries(${name} INTERFACE ${name}_sse_or_arm ${name}_avx)
endfunction()
