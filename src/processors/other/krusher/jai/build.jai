#import "Compiler";
#import "generate_c_header";

#run build();

build :: () {
    header_info : Header_Info;
    header_info.jai_type_prefix = "jai_";

    w := compiler_create_workspace();

    target_options := get_build_options(w);
    target_options.output_executable_name = "krusher_jai_lib";
    target_options.output_type = .STATIC_LIBRARY; // specifies output to be a static library

    set_build_options(target_options, w);

    compiler_begin_intercept(w);
    add_build_file("lofi_downsampler.jai", w);
    while true {
        message := compiler_wait_for_message();
        handle_message(*header_info, message);
        if message.kind == {
          case .COMPLETE;
            break;
        }
    }
    compiler_end_intercept(w);

    generate_header(*header_info, "krusher_jai_lib.h");

    set_build_options_dc(.{do_output=false});  // No executable for this workspace.
}
