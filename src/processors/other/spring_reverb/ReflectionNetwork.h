#pragma once

#include <pch.h>

class ReflectionNetwork
{
public:
    ReflectionNetwork() = default;

    void prepare (const dsp::ProcessSpec& spec)
    {
        fs = (float) spec.sampleRate;

        for (auto& d : delays)
            d.prepare (spec);

        shelfFilter.reset();
    }

    void reset()
    {
        for (auto& d : delays)
            d.reset();
    }

    void setParams (float reverbSize, float t60, float mix, float damping)
    {
        constexpr float baseDelaysSec[4] = { 0.07f, 0.17f, 0.23f, 0.29f };

        float delaySamples alignas (16)[4];
        for (int i = 0; i < 4; ++i)
        {
            delaySamples[i] = baseDelaysSec[i] * reverbSize * fs;
            delays[(size_t) i].setDelay (delaySamples[i]);
        }

        using namespace chowdsp::SIMDUtils;
        auto delaySamplesVec = VecType::fromRawArray (delaySamples);
        feedback = powSIMD (VecType (0.001f * std::pow (10.0f, mix)), delaySamplesVec / VecType (t60 * fs));
        feedback *= 0.23f * mix * (0.735f + 0.235f * reverbSize);

        float dampDB = -3.0f - 9.0f * damping;
        shelfFilter.calcCoefs (1.0f, Decibels::decibelsToGain (dampDB), 800.0f, fs);
    }

    inline float popSample (int ch) noexcept
    {
        float output alignas (16)[4];
        for (int i = 0; i < 4; ++i)
            output[i] = delays[(size_t) i].popSample (ch);

        auto outVec = VecType::fromRawArray (output);

        // householder matrix
        constexpr auto householderFactor = -2.0f / (float) VecType::size();
        outVec += outVec.sum() * householderFactor;

        outVec *= feedback;
        return shelfFilter.processSample (outVec.sum() / (float) VecType::size());
    }

    inline void pushSample (int ch, float x) noexcept
    {
        for (auto& d : delays)
            d.pushSample (ch, x);
    }

private:
    using VecType = dsp::SIMDRegister<float>;
    using ReflectionDelay = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd>;
    std::array<ReflectionDelay, 4> delays { ReflectionDelay { 1 << 18 },
                                            ReflectionDelay { 1 << 18 },
                                            ReflectionDelay { 1 << 18 },
                                            ReflectionDelay { 1 << 18 } };

    VecType feedback {};
    float fs = 48000.0f;

    chowdsp::ShelfFilter<float> shelfFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReflectionNetwork)
};
