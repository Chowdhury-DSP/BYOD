#import "Basic";
#import "Compiler";
#import "generate_c_header";

#run build();

SRC_FILES :: string.[
    "krusher/lofi_downsampler.jai",
    "krusher/bit_reduction.jai"
];

build :: () {
    header_info : Header_Info;
    header_info.jai_type_prefix = "jai_";

    w := compiler_create_workspace();

    target_options := get_build_options(w);
    target_options.output_executable_name = "byod_jai_lib";
    target_options.output_type = .STATIC_LIBRARY; // specifies output to be a static library
    target_options.backend = .LLVM;
    target_options.llvm_options.command_line = string.[target_options.output_executable_name, "--relocation-model=pic"];
    target_options.text_output_flags = 1;
    set_optimization(*target_options, .OPTIMIZED);

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

    generate_header(*header_info, "byod_jai_lib.h");

    set_build_options_dc(.{do_output=false});  // No executable for this workspace.
}
