Krusher_Bit_Reducer_Filter_State :: struct {
    p1: s32;
    p2: s32;
}

#program_export
krusher_bit_reduce_process_block :: (buffer: **float,
                                     num_channels: s32,
                                     num_samples: s32,
                                     filter_index: s32,
                                     bit_depth: s32,
                                     filter_states: *Krusher_Bit_Reducer_Filter_State) #c_call {
    small_block_size : s32 : 16;
    samples_int : [small_block_size]s16 = ---;

    for channel : 0..num_channels-1 {
        samples_remaining : s32 = num_samples;
        while samples_remaining > 0 {
            samples_to_process := ifx samples_remaining > small_block_size then small_block_size else samples_remaining;
            defer { samples_remaining -= samples_to_process; }

            samples_float_span : []float32;
            samples_float_span.data = buffer[channel] + num_samples - samples_remaining;
            samples_float_span.count = samples_to_process;

            memset(*samples_int, 0, size_of(s16) * small_block_size);
            samples_int_span : []s16;
            samples_int_span.data = samples_int.data;
            samples_int_span.count = samples_to_process;

            convert_float_to_int(samples_float_span, samples_int_span);
            
            if (bit_depth < 12) {
                br_data := bit_reduce_encode(samples_int_span, bit_depth);
                bit_reduce_decode(br_data, samples_int_span, cast(BR_Filter) filter_index, *filter_states[channel]);
            }
            
            convert_int_to_float(samples_int_span, samples_float_span);
        }
    }
}

#scope_file
BIT_MASKS :: u16.[
    0, // 0
    0x0001, // 1
    0x0003, // 2
    0x0007, // 3
    0x000F, // 4
    0x001F, // 5
    0x003F, // 6
    0x007F, // 7
    0x00FF, // 8
    0x01FF, // 9
    0x03FF, // 10
    0x07FF, // 11
    0x0FFF, // 12
    0x1FFF, // 13
    0x3FFF, // 14
    0x7FFF, // 15
];

Bit_Reduction_Block :: struct {
    shift_amount: u8;
    data: [16] u16;
}

BR_Filter :: enum {
    TYPE_0;
    TYPE_1;
    TYPE_2;
    TYPE_3;
}

encode_sample :: inline (shift: u8, bit_depth: s32, x: s16) -> u16 #no_context {
    value_unsigned := cast(u16) (x + (1 << 8));
    return cast(u16) (value_unsigned >> shift) & BIT_MASKS[bit_depth];
}

decode_sample :: inline (shift: u8, x: u16) -> s16 #no_context {
    return cast(s16) (cast(u16) (x << shift) - (1 << 8));
}

bit_reduce_decode :: (using br_block: Bit_Reduction_Block,
                      out: []s16,
                      filter: BR_Filter,
                      state: *Krusher_Bit_Reducer_Filter_State) #no_context {

    type1_filter :: inline (nibble_2r: s16, using state: *Krusher_Bit_Reducer_Filter_State) -> s16 #no_context {
        y := cast(s32) nibble_2r + ((p1 * 15) >> 4);
        p2 = 0;
        p1 = y;
        return cast(s16) (y >> 4);
    }

    type2_filter :: inline (nibble_2r: s16, using state: *Krusher_Bit_Reducer_Filter_State) -> s16 #no_context {
        y := cast(s32) nibble_2r + ((p1 * 61) >> 5) -  ((p2 * 15) >> 4);
        p2 = p1;
        p1 = y;
        return cast(s16) (y >> 5);
    }

    type3_filter :: inline (nibble_2r: s16, using state: *Krusher_Bit_Reducer_Filter_State) -> s16 #no_context {
        y := cast(s32) nibble_2r + ((p1 * 115) >> 6) -  ((p2 * 13) >> 4);
        p2 = p1;
        p1 = y;
        return cast(s16) (y >> 6);
    }

    for br_sample, i : data {
        if #complete filter == {
            case .TYPE_0; out[i] = decode_sample(shift_amount, br_sample);
            case .TYPE_1; out[i] = type1_filter (decode_sample(shift_amount, br_sample), state);
            case .TYPE_2; out[i] = type2_filter (decode_sample(shift_amount, br_sample), state);
            case .TYPE_3; out[i] = type3_filter (decode_sample(shift_amount, br_sample), state);
        }
    }
}

bit_reduce_encode :: (pcm_data: []s16, bit_depth: s32) -> Bit_Reduction_Block #no_context {
    shift_best : u8 = 0;
    err_min : float64 = Math.FLOAT64_MAX;

    for s : cast(u8) 0.. cast(u8) (16 - bit_depth) {
        err_square_accum : float64 = 0.0;
        for pcm_sample, _ : pcm_data {
            pred := decode_sample(s, encode_sample(s, bit_depth, pcm_sample));
            err := cast(float64) (pcm_sample - pred);
            err_square_accum += err * err;
        }

        if err_square_accum < err_min {
            err_min = err_square_accum;
            shift_best = s;
        }
    }

    using br_block : Bit_Reduction_Block = ---;
    shift_amount = shift_best;
    for pcm_sample, i : pcm_data {
        data[i] = encode_sample(shift_best, bit_depth, pcm_sample);
    }

    return br_block;
}

convert_float_to_int :: (data_float: [] float, data_int: [] s16) #no_context {
    for float_sample, i : data_float {
        data_int[i] = cast(s16) (float_sample * cast(float32) (1 << 8));
    }
}

convert_int_to_float :: (data_int: [] s16, data_float: [] float) #no_context {
    for _, i : data_float {
        data_float[i] = cast(float32) data_int[i] / cast(float32) (1 << 8);
    }
}

Math :: #import "Math";