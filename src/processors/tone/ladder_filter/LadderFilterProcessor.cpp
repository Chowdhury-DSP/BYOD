#include "../../ParameterHelpers.h"

#include "LadderFilterProcessor.h"

LadderFilterProcessor::LadderFilterProcessor (UndoManager* um) : BaseProcessor ("Ladder Filter", LadderParameters::createParameterLayout(), um), p (vts)
{
    // Info and style
    uiOptions.info.description = "Resonant 4-pole ladder high-pass and low-pass,\n capable of self-oscillation. With nonlinear drive.";
    uiOptions.info.authors = StringArray { "Butch Warns" };
    uiOptions.info.infoLink = "https://butchwarns.de/";
    uiOptions.backgroundColour = juce::Colour::fromRGB (0xBB, 0xE1, 0xC3);
    uiOptions.powerColour = juce::Colour::fromRGB (0x99, 0x0B, 0x3C);

    addPopupMenuParameter ("FILTER_MODE");
}

void LadderFilterProcessor::prepare (double sampleRate, int samplesPerBlock)
{
    // Setup parameter denormalization / skew

    drive_smooth.mappingFunction = [] (float val)
    {
        const double decibels = ladder_filter_utility::map_linear_normalized (static_cast<double> (val), -24.0, 24.0);
        const double gain_factor = ladder_filter_utility::decibel_to_raw_gain (decibels);

        return gain_factor;
    };

    hp_cutoff_smooth.mappingFunction = [] (float val)
    {
        const double control_voltage = ladder_filter_utility::map_linear_normalized (static_cast<double> (val), -5.0, 5.0);
        const double cutoff = ladder_filter_utility::volt_to_freq (control_voltage);

        return cutoff;
    };

    hp_resonance_smooth.mappingFunction = [this] (float val)
    {
        if (*(p.filter_mode_norm) == 0.0f)
        {
            val = ladder_filter_utility::limit_upper (val, p.RESONANCE_LIMIT_NON_OSCILLATING);
        }

        double resonance = ladder_filter_utility::skew_normalized (static_cast<double> (val), 0.33);
        resonance = ladder_filter_utility::map_linear_normalized (resonance, 0.0, 4.0);

        return resonance;
    };

    lp_cutoff_smooth.mappingFunction = [] (float val)
    {
        const double control_voltage = ladder_filter_utility::map_linear_normalized (static_cast<double> (val), -5.0, 5.0);
        const double cutoff = ladder_filter_utility::volt_to_freq (control_voltage);

        return cutoff;
    };

    lp_resonance_smooth.mappingFunction = [this] (float val)
    {
        if (*(p.filter_mode_norm) == 0.0f)
        {
            val = ladder_filter_utility::limit_upper (val, p.RESONANCE_LIMIT_NON_OSCILLATING);
        }

        double resonance = ladder_filter_utility::skew_normalized (static_cast<double> (val), 0.33);
        resonance = ladder_filter_utility::map_linear_normalized (resonance, 0.0, 4.0);

        return resonance;
    };

    drive_smooth.setParameterHandle (p.drive_norm);
    drive_norm_smooth.setParameterHandle (p.drive_norm);
    hp_cutoff_smooth.setParameterHandle (p.hp_cutoff_norm);
    hp_resonance_smooth.setParameterHandle (p.hp_resonance_norm);
    lp_cutoff_smooth.setParameterHandle (p.lp_cutoff_norm);
    lp_resonance_smooth.setParameterHandle (p.lp_resonance_norm);

    drive_smooth.prepare (sampleRate, samplesPerBlock);
    drive_norm_smooth.prepare (sampleRate, samplesPerBlock);
    hp_cutoff_smooth.prepare (sampleRate, samplesPerBlock);
    hp_resonance_smooth.prepare (sampleRate, samplesPerBlock);
    lp_cutoff_smooth.prepare (sampleRate, samplesPerBlock);
    lp_resonance_smooth.prepare (sampleRate, samplesPerBlock);

    // Initialize filters
    for (int channel = 0; channel < 2; ++channel)
    {
        lp[channel].reset (sampleRate);
        hp[channel].reset (sampleRate);
    }
}

void LadderFilterProcessor::processAudio (AudioBuffer<float>& buffer)
{
    const int num_samples = buffer.getNumSamples();

    // Process parameter smoothers for current buffer
    hp_cutoff_smooth.process (num_samples);
    hp_resonance_smooth.process (num_samples);
    lp_cutoff_smooth.process (num_samples);
    lp_resonance_smooth.process (num_samples);
    drive_smooth.process (num_samples);
    drive_norm_smooth.process (num_samples);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* data = buffer.getWritePointer (channel);

        // Process high-pass

        // If we need smoothing..
        if (hp_cutoff_smooth.isSmoothing() || hp_resonance_smooth.isSmoothing())
        {
            // ..update filter parameters
            const float* cutoff_smoothed = hp_cutoff_smooth.getSmoothedBuffer();
            const float* resonance_smoothed = hp_resonance_smooth.getSmoothedBuffer();

            for (int n = 0; n < num_samples; ++n)
            {
                hp[channel].set_cutoff (cutoff_smoothed[n]);
                hp[channel].set_resonance (resonance_smoothed[n]);

                float& x = data[n];
                x = hp[channel].process (x);
            }
        }
        else
        {
            // No need for smoohting parameters
            hp[channel].set_cutoff (hp_cutoff_smooth.getCurrentValue());
            hp[channel].set_resonance (hp_resonance_smooth.getCurrentValue());

            for (int n = 0; n < num_samples; ++n)
            {
                float& x = data[n];
                x = hp[channel].process (x);
            }
        }

        // Process low-pass

        // If we need smoothing..
        if (lp_cutoff_smooth.isSmoothing() || lp_resonance_smooth.isSmoothing())
        {
            // ..update filter parameters
            const float* cutoff_smoothed = lp_cutoff_smooth.getSmoothedBuffer();
            const float* resonance_smoothed = lp_resonance_smooth.getSmoothedBuffer();

            for (int n = 0; n < num_samples; ++n)
            {
                lp[channel].set_cutoff (cutoff_smoothed[n]);
                lp[channel].set_resonance (resonance_smoothed[n]);

                float& x = data[n];
                x = lp[channel].process (x);
            }
        }
        else
        {
            // No need for smoohting parameters
            lp[channel].set_cutoff (lp_cutoff_smooth.getCurrentValue());
            lp[channel].set_resonance (lp_resonance_smooth.getCurrentValue());

            for (int n = 0; n < num_samples; ++n)
            {
                float& x = data[n];
                x = lp[channel].process (x);
            }
        }

        // apply drive
        juce::FloatVectorOperations::multiply (data, drive_smooth.getSmoothedBuffer(), num_samples);

        // saturate and blend
        const float* drive_norm_smoothed = drive_norm_smooth.getSmoothedBuffer();
        for (int n = 0; n < num_samples; ++n)
        {
            float& x = data[n];
            x = drive_norm_smoothed[n] * ladder_filter_utility::fast_tanh_2 (x) + (1.0f - drive_norm_smoothed[n]) * x;
        }
    }
}