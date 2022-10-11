#pragma once

struct CleanDelayType
{
    void prepare (const dsp::ProcessSpec& spec)
    {
        lpf.prepare (spec);
        delay.prepare (spec);
    }

    void reset()
    {
        lpf.reset();
        delay.reset();
    }

    void setDelay (float newDelayInSamples) { delay.setDelay (newDelayInSamples); }
    void setFilterFreq (float freqHz) { lpf.setCutoffFrequency (freqHz); }

    inline void pushSample (int channel, float sample) { delay.pushSample (channel, sample); }
    inline float popSample (int channel) { return lpf.processSample (channel, delay.popSample (channel)); }

    chowdsp::SVFLowpass<float> lpf;
    chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th> delay { 1 << 18 };
};
