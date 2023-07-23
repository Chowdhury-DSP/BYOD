Lofi_Resample_State :: struct {
    upsample_overshoot: float64;
    downsample_overshoot: float64;
}

#program_export
krusher_init_lofi_resample :: (using state: *Lofi_Resample_State) #c_call {
    upsample_overshoot = 0.0;
    downsample_overshoot = 0.0;
}

#program_export
krusher_process_lofi_downsample :: (ctx: *Context,
                                    using state: *Lofi_Resample_State,
                                    buffer: **float32,
                                    num_channels: s32,
                                    num_samples: s32,
                                    resample_factor: float64) #c_call
{
    push_context ctx {
        ds_buffer_size := cast(s32) Math.ceil(cast(float64) num_samples / resample_factor);

        // allocating memory here, but it's real-time safe since we're using Temporary_Storage
        temp_data : [..] float32;
        temp_data.allocator = temp;
        array_resize(*temp_data, 2 * ds_buffer_size, true);

        // Eventually we should figure out a cleaner way to have a kind of "audio buffer view"
        ds_buffer : [2] *float32;
        ds_buffer[0] = temp_data.data;
        ds_buffer[1] = temp_data.data + ds_buffer_size;

        krusher_process_lofi_resample(buffer, ds_buffer.data, num_channels, num_samples, ds_buffer_size, resample_factor, *downsample_overshoot);
        krusher_process_lofi_resample(ds_buffer.data, buffer, num_channels, ds_buffer_size, num_samples, 1.0 / resample_factor, *upsample_overshoot);

        reset_temporary_storage();
    }
}

#scope_file
krusher_process_lofi_resample :: (source_buffer: **float32,
                                  dest_buffer: **float32,
                                  num_channels: s32,
                                  num_samples_source: s32,
                                  num_samples_dest: s32,
                                  resample_factor: float64,
                                  overshoot_samples: *float64)
{
    // simple S&H lofi resampler
    grab_index : s32 = -1;
    for channel: 0..num_channels-1 {
        source_data := source_buffer[channel];
        dest_data := dest_buffer[channel];

        for i: 0..num_samples_dest-1 {
            grab_index = cast(s32) (cast(float64) i * resample_factor + (<<overshoot_samples));
            dest_data[i] = source_data[min(grab_index, num_samples_source - 1)];
        }
    }

    <<overshoot_samples = cast(float64) num_samples_dest * resample_factor - Math.floor(cast(float64) num_samples_dest * resample_factor);
}

#import "Basic";
Math :: #import "Math";
