#include "../../ParameterHelpers.h"

#include "LadderFilter.h"

LadderFilter::LadderFilter (UndoManager* um) : BaseProcessor ("Ladder Filter", LadderParameters::createParameterLayout(), um), p (vts)
{
    // Info and style
    uiOptions.info.description = "Resonant 4-pole ladder high-pass and low-pass,\n capable of self-oscillation. With nonlinear drive.";
    uiOptions.info.authors = StringArray { "Butch Warns" };
    uiOptions.info.infoLink = "https://butchwarns.de/";
    uiOptions.backgroundColour = juce::Colour::fromRGB (0xBB, 0xE1, 0xC3);
    uiOptions.powerColour = juce::Colour::fromRGB (0x99, 0x0B, 0x3C);
}

void LadderFilter::prepare (double sampleRate, int samplesPerBlock)
{
    // Initialize filters
    for (int channel = 0; channel < 2; ++channel)
    {
        lp[channel].reset (sampleRate);
        hp[channel].reset (sampleRate);
    }

    // Initialize parameter smoothing
    p.reset (sampleRate, samplesPerBlock);
}

void LadderFilter::processAudio (AudioBuffer<float>& buffer)
{
    // Get parameter values for current block

    const double hp_cutoff = p.hp_cutoff();
    const double hp_resonance = p.hp_resonance();
    const double lp_cutoff = p.lp_cutoff();
    const double lp_resonance = p.lp_resonance();

    const double gain = p.drive();

    // Update filter parameters
    for (int channel = 0; channel < 2; ++channel)
    {
        hp[channel].set_cutoff (hp_cutoff);
        hp[channel].set_resonance (hp_resonance);
        lp[channel].set_cutoff (lp_cutoff);
        lp[channel].set_resonance (lp_resonance);
    }

    // Iterate over frames
    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        // Process samples in the current frame
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            // Reference the input sample for cleaner calculations
            float& x = buffer.getWritePointer (channel)[n];

            //Apply filters
            x = hp[channel].process (x);
            x = lp[channel].process (x);

            // Appply drive
            x *= gain;

            // Saturate (static nonlinearity) and blend using drive param
            x = p.drive_normalized() * fast_tanh_2 (x) + (1.0 - p.drive_normalized()) * x;
        }
    }
}