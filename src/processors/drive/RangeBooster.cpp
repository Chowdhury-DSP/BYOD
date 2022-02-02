#include "RangeBooster.h"
#include "../ParameterHelpers.h"

RangeBooster::RangeBooster (UndoManager* um) : BaseProcessor ("Range Booster", createParameterLayout(), um)
{
    rangeParam = vts.getRawParameterValue ("range");
    boostParam = vts.getRawParameterValue ("boost");

    uiOptions.backgroundColour = Colours::grey.brighter (0.7f);
    uiOptions.powerColour = Colours::orangered.brighter(0.1f);
    uiOptions.info.description = "Range booster effect inspired by the Dallas Rangemaster pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout RangeBooster::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createFreqParameter (params, "range", "Range", 250.0f, 5000.0f, 2600.0f, 2600.0f);
    createGainDBParameter (params, "boost", "Boost", -30.0f, 12.0f, 0.0f);

    return { params.begin(), params.end() };
}

void RangeBooster::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    for (int ch = 0; ch < 2; ++ch)
    {
        freqSmooth[ch].reset (sampleRate, 0.02);
        freqSmooth[ch].setCurrentAndTargetValue (*rangeParam);
        inputFilter[ch].reset();

        veState[ch] = 8.4;
        c3State[ch] = 0.0;
    }

    constexpr float C3 = 47e-6f;
    constexpr float R3 = 3.9e3f;
    c3Coefs[0] = double (C3 * fs + 1.0f / R3);
    c3Coefs[1] = double (C3 * fs);

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    outGain.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    outGain.setRampDurationSeconds (0.02);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 10000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void RangeBooster::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    auto&& block = dsp::AudioBlock<float> { buffer };
    auto&& context = dsp::ProcessContextReplacing<float> { block };

    // set up input range
    block *= 0.2f;
    for (int ch = 0; ch < numChannels; ++ch)
        FloatVectorOperations::clip (buffer.getWritePointer (ch), buffer.getReadPointer (ch), -1.0f, 1.0f, numSamples);

    // process input filter
    for (int ch = 0; ch < numChannels; ++ch)
    {
        freqSmooth[ch].setTargetValue (*rangeParam);
        auto* x = buffer.getWritePointer (ch);

        if (freqSmooth[ch].isSmoothing())
        {
            for (int n = 0; n < numSamples; ++n)
            {
                inputFilter[ch].calcCoefs (freqSmooth[ch].getNextValue(), fs);
                x[n] = inputFilter[ch].processSample (x[n]);
            }
        }
        else
        {
            inputFilter[ch].calcCoefs (freqSmooth[ch].getNextValue(), fs);
            inputFilter[ch].processBlock (x, numSamples);
        }
    }

    // process PNP nonlinearity
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < numSamples; ++n)
        {
            static constexpr double V_plus = 9.0;
            static constexpr double R1 = 470e3;
            static constexpr double R2 = 68e3;
            static constexpr double RV = 10e3;
            static constexpr double R_bias = R1 / (R1 + R2);
            static constexpr double Z_T = 12.5e3;

            static constexpr double Vt = 0.02585;
            static constexpr double I_s = 10.0e-15;
            static constexpr double Beta_F = 200.0;
            static constexpr double Beta_R = 2.0;

            const double v_b = (double) x[n] + R_bias * V_plus;
            const double i_b = v_b / Z_T;

            const double i_e = c3Coefs[0] * (V_plus - veState[ch]) + c3Coefs[1] * c3State[ch];
            c3State[ch] = i_e;

            const double v_be = v_b - veState[ch];
            const double exp_v_be = std::exp(v_be / Vt);

            double v_bc = Vt * std::log(((i_b / I_s) - (1.0 / Beta_F) * (exp_v_be - 1.0)) * Beta_R + 1.0);
            double i_c = 0.0;
            for (int k = 0; k < 5; ++k)
            {
                const double exp_v_bc = std::exp(v_bc / Vt);
                i_c = I_s * ((exp_v_be - exp_v_bc) - (1.0 / Beta_R) * (exp_v_bc - 1.0));

                double F_y = i_b + i_e - i_c;
                double dF_y = -I_s * ((-1.0 / Vt) * exp_v_bc - (1.0 / Vt / Beta_R) * exp_v_bc);

                v_bc -= F_y / (dF_y + 1.0e-24);
            }

            const double exp_v_bc = std::exp(v_bc / Vt);
            veState[ch] = v_b - Vt * std::log((i_c / I_s) + (1.0 / Beta_R) * (exp_v_bc - 1.0) + exp_v_bc);

            x[n] = float ((i_c * RV) * 1e16 - 5e5 - 1.0);
        }
    }

    dcBlocker.processAudio (buffer);

    // process output level
    outGain.setGainDecibels (*boostParam + 56.0f);
    outGain.process (context);
}
