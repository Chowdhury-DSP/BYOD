#pragma once

#include <pch.h>

class KingOfToneClipper
{
public:
    KingOfToneClipper() = default;

    void processBlock (float* x, int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
        {
            R12_Vs.setVoltage (x[n]);

            dp.incident (S1.reflected());
            S1.incident (dp.reflected());

            x[n] = wdft::voltage<float> (dp);
        }
    }

    wdft::ResistiveVoltageSourceT<float> R12_Vs { 1.0e3f };
    wdft::PolarityInverterT<float, decltype (R12_Vs)> S1 { R12_Vs };
    wdft::DiodePairT<float, decltype (S1)> dp { S1, 2.52e-9f, 25.85e-3f, 1.752f };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KingOfToneClipper)
};
