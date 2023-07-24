#pragma once

struct Krusher_Lofi_Resample_State
{
    double upsample_overshoot = 0.0;
    double downsample_overshoot = 0.0;
};

inline void krusher_init_lofi_resample (Krusher_Lofi_Resample_State* state)
{
    state->upsample_overshoot = 0.0;
    state->downsample_overshoot = 0.0;
}

inline void krusher_process_lofi_resample (float** source_buffer,
                                           float** dest_buffer,
                                           int num_channels,
                                           int num_samples_source,
                                           int num_samples_dest,
                                           double resample_factor,
                                           double& overshoot_samples)
{
    // simple S&H lofi resampler
    for (int channel = 0; channel < num_channels; ++channel)
    {
        const auto* source_data = source_buffer[channel];
        auto* dest_data = dest_buffer[channel];

        for (int i = 0; i < num_samples_dest; ++i)
        {
            const auto grab_index = (int) ((double) i * resample_factor + overshoot_samples);
            dest_data[i] = source_data[std::min (grab_index, num_samples_source - 1)];
        }
    }

    overshoot_samples = (double) num_samples_dest * resample_factor - std::floor ((double) num_samples_dest * resample_factor);
}

inline void krusher_process_lofi_downsample ([[maybe_unused]] void* ctx,
                                             Krusher_Lofi_Resample_State* state,
                                             float** buffer,
                                             int num_channels,
                                             int num_samples,
                                             double resample_factor)
{
    const auto ds_buffer_size = (int) std::ceil ((double) num_samples / resample_factor);

    auto* temp_data = (float*) alloca (2 * (size_t) ds_buffer_size * sizeof (float));
    float* ds_buffer[2] = { temp_data, temp_data + ds_buffer_size };

    krusher_process_lofi_resample (buffer, ds_buffer, num_channels, num_samples, ds_buffer_size, resample_factor, state->downsample_overshoot);
    krusher_process_lofi_resample (ds_buffer, buffer, num_channels, ds_buffer_size, num_samples, 1.0 / resample_factor, state->upsample_overshoot);
}
