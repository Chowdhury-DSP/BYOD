#import "Basic";
#import "Compiler";
#import "generate_cpp_header";

SRC_FILES :: string.[
    "krusher/lofi_downsampler.jai",
    "krusher/bit_reduction.jai"
];

build :: (building_arm64 := false, gen_header := true, lib_postfix := "") {
    header_info : Header_Info;

    w := compiler_create_workspace();

    target_options := get_build_options(w);
    target_options.output_executable_name = tprint("%0%", "byod_jai_lib", lib_postfix);
    target_options.output_type = .STATIC_LIBRARY; // specifies output to be a static library
    target_options.backend = .LLVM;
    target_options.text_output_flags = 1;
    set_optimization(*target_options, .OPTIMIZED);
    if building_arm64 {
        target_options.cpu_target = .ARM64;
        #if TARGET_OS == "IOS" {
            target_options.llvm_options.target_system_triple = "arm64-apple-ios9.0";
        } else {
            target_options.llvm_options.target_system_triple = "arm64-apple-darwin20.1.0";
        }
    }

    set_build_options(target_options, w);

    compiler_begin_intercept(w);
    for file, _ : SRC_FILES {
        add_build_file(tprint("%/%", #filepath, file), w);
    }
    while true {
        message := compiler_wait_for_message();
        handle_message(*header_info, message);
        if message.kind == {
          case .COMPLETE;
            break;
        }
    }
    compiler_end_intercept(w);

    if gen_header {
        generate_header(*header_info, "byod_jai_lib.h");
    }

    set_build_options_dc(.{do_output=false});  // No executable for this workspace.
}

#if TARGET_OS == "IOS" {
    #run {
        print("Building arm64 binary on iOS...\n");
        build(true, true, "_ios");
    }
}
#if TARGET_OS == "MACOS" {
    #run {
        print("Building universal binary on MacOS...\n");

        arm_lib := "byod_jai_lib_arm64.a";
        x64_lib := "byod_jai_lib_x64.a";

        File.file_delete(arm_lib);
        File.file_delete(x64_lib);

        build(false, true, "_x64");
        build(true, false, "_arm64");

        // For some reason we need to wait a second for the ARM build to finish?
        // sleep_milliseconds(500);

        lipo_command := string.["lipo", "-create", "-output", "byod_jai_lib.a", x64_lib, arm_lib];
        Process.run_command(..lipo_command);

        lipo_check_command := string.["lipo", "-info", "byod_jai_lib.a"];
        Process.run_command(..lipo_check_command);
    }

    Process :: #import "Process";
    File :: #import "File";
}
#if TARGET_OS == "WINDOWS" || TARGET_OS == "LINUX" {
    #run build();
}
