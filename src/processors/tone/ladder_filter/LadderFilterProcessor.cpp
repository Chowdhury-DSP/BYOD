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
    typedef LadderParameters params;
    drive_smooth.mappingFunction = params::drive;
    hp_cutoff_smooth.mappingFunction = params::hp_cutoff;
    hp_resonance_smooth.mappingFunction = params::hp_resonance;
    lp_cutoff_smooth.mappingFunction = params::lp_cutoff;
    lp_resonance_smooth.mappingFunction = params::lp_resonance;

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

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* data = buffer.getWritePointer (channel);

        // Process high-pass

        // If we need smoothing..
        hp_cutoff_smooth.process (num_samples);
        hp_resonance_smooth.process (num_samples);
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
        lp_cutoff_smooth.process (num_samples);
        lp_resonance_smooth.process (num_samples);
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
        drive_smooth.process (num_samples);
        juce::FloatVectorOperations::multiply (data, drive_smooth.getSmoothedBuffer(), num_samples);

        // saturate and blend
        drive_norm_smooth.process (num_samples);
        const float* drive_norm_smoothed = drive_norm_smooth.getSmoothedBuffer();
        for (int n = 0; n < num_samples; ++n)
        {
            float& x = data[n];
            x = drive_norm_smoothed[n] * ladder_filter_utility::fast_tanh_2 (x) + (1.0f - drive_norm_smoothed[n]) * x;
        }
    }
}